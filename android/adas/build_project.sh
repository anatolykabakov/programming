#!/bin/bash

# Скрипт сборки проекта ADAS (C++ + APK)

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="debug"
CLEAN_BUILD=false
BUILD_CPP_ONLY=false
VERBOSE=false

print_status()  { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }

read_local_properties() {
    local props="${PROJECT_DIR}/local.properties"
    if [ ! -f "$props" ]; then
        return
    fi
    local sdk ndk
    sdk="$(grep -E '^sdk\.dir=' "$props" | head -1 | cut -d= -f2-)"
    ndk="$(grep -E '^ndk\.dir=' "$props" | head -1 | cut -d= -f2-)"
    # Prefer local.properties when env is missing or points to a non-existent path
    if [ -n "$sdk" ] && { [ -z "${ANDROID_HOME:-}" ] || [ ! -d "${ANDROID_HOME}" ]; }; then
        export ANDROID_HOME="$sdk"
    fi
    if [ -n "$ndk" ] && { [ -z "${ANDROID_NDK_ROOT:-}" ] || [ ! -d "${ANDROID_NDK_ROOT}" ]; }; then
        export ANDROID_NDK_ROOT="$ndk"
        export ANDROID_NDK_HOME="$ndk"
    fi
}

resolve_java_home() {
    if [ -n "${JAVA_HOME:-}" ] && [ -x "${JAVA_HOME}/bin/java" ]; then
        return
    fi
    local portable="${PROJECT_DIR}/.tools/jdk-21"
    if [ -d "$portable" ]; then
        local jdk
        jdk="$(find "$portable" -maxdepth 2 -type d -name 'jdk-*' 2>/dev/null | head -1)"
        if [ -n "$jdk" ] && [ -x "$jdk/bin/java" ]; then
            export JAVA_HOME="$jdk"
            return
        fi
    fi
    for candidate in \
        /usr/lib/jvm/java-21-openjdk-amd64 \
        /usr/lib/jvm/java-17-openjdk-amd64 \
        /usr/lib/jvm/default-java; do
        if [ -x "$candidate/bin/java" ]; then
            export JAVA_HOME="$candidate"
            return
        fi
    done
}

show_help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo
    echo "ОПЦИИ:"
    echo "  -t, --type TYPE        Тип сборки (debug|release) [по умолчанию: debug]"
    echo "  -c, --clean            Очистить перед сборкой"
    echo "  --cpp-only             Собрать только C++ часть"
    echo "  -v, --verbose          Подробный вывод"
    echo "  -h, --help             Показать эту справку"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            --cpp-only)
                BUILD_CPP_ONLY=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "Неизвестная опция: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

check_dependencies() {
    print_status "Проверка зависимостей..."
    print_status "PROJECT_DIR=$PROJECT_DIR"

    resolve_java_home
    if ! command -v java &> /dev/null && [ ! -x "${JAVA_HOME:-}/bin/java" ]; then
        print_error "Java не найдена. Запустите install_dependencies.sh"
        exit 1
    fi

    read_local_properties

    local sdk="${ANDROID_HOME:-/usr/lib/android-sdk}"
    if [ ! -d "$sdk" ]; then
        print_error "Android SDK не найден ($sdk). Запустите install_dependencies.sh"
        exit 1
    fi
    export ANDROID_HOME="$sdk"

    if [ -z "${ANDROID_NDK_ROOT:-}" ] || [ ! -d "$ANDROID_NDK_ROOT" ]; then
        print_error "Android NDK не найден (ANDROID_NDK_ROOT='${ANDROID_NDK_ROOT:-}')."
        print_error "Запустите: ./install_dependencies.sh --local-properties-only"
        exit 1
    fi

    if [ ! -f "$PROJECT_DIR/gradlew" ]; then
        print_error "Gradle wrapper не найден в $PROJECT_DIR"
        exit 1
    fi

    print_success "Зависимости OK (SDK=$ANDROID_HOME, NDK=$ANDROID_NDK_ROOT)"
}

setup_environment() {
    print_status "Настройка переменных окружения..."
    resolve_java_home
    read_local_properties
    export ANDROID_HOME="${ANDROID_HOME:-/usr/lib/android-sdk}"
    export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
    export PATH="${JAVA_HOME}/bin:${ANDROID_HOME}/platform-tools:${PATH}"
    mkdir -p "${PROJECT_DIR}/.tools/gradle-home"
    export GRADLE_USER_HOME="${PROJECT_DIR}/.tools/gradle-home"
    print_success "JAVA_HOME=$JAVA_HOME"
    print_success "ANDROID_HOME=$ANDROID_HOME"
    print_success "ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT"
}

clean_project() {
    if [ "$CLEAN_BUILD" = true ]; then
        print_status "Очистка проекта..."
        cd "$PROJECT_DIR"
        ./gradlew clean
        print_success "Проект очищен"
    fi
}

build_cpp() {
    print_status "Сборка C++ библиотеки..."
    cd "$PROJECT_DIR/app/src/main/cpp"
    local args=(-t android)
    if [ "$CLEAN_BUILD" = true ]; then
        args+=(-c)
    fi
    if [ "$VERBOSE" = true ]; then
        args+=(-v)
    fi
    ./build_cpp.sh "${args[@]}"
    cd "$PROJECT_DIR"
}

build_android() {
    print_status "Сборка Android приложения..."
    cd "$PROJECT_DIR"

    local gradle_task="assembleDebug"
    if [ "$BUILD_TYPE" = "release" ]; then
        gradle_task="assembleRelease"
    fi

    print_status "Выполнение: ./gradlew $gradle_task"
    if [ "$VERBOSE" = true ]; then
        ./gradlew "$gradle_task" --info
    else
        ./gradlew "$gradle_task"
    fi

    local apk_dir="app/build/outputs/apk/$BUILD_TYPE"
    if [ -d "$apk_dir" ] && ls "$apk_dir"/*.apk &>/dev/null; then
        print_success "APK создан в: $apk_dir"
        ls -la "$apk_dir"/*.apk
    else
        print_error "Ошибка: APK не создан"
        return 1
    fi
}

show_build_info() {
    echo
    print_status "Информация о сборке:"
    echo "  - PROJECT_DIR: $PROJECT_DIR"
    echo "  - Тип сборки: $BUILD_TYPE"
    echo "  - Очистка: $CLEAN_BUILD"
    echo "  - Только C++: $BUILD_CPP_ONLY"
    echo "  - Подробный вывод: $VERBOSE"
    echo
}

main() {
    echo "=========================================="
    echo "  Сборка проекта ADAS"
    echo "=========================================="
    echo

    parse_arguments "$@"
    show_build_info

    check_dependencies
    setup_environment
    clean_project
    build_cpp

    if [ "$BUILD_CPP_ONLY" = false ]; then
        build_android
    fi

    echo
    print_success "Сборка завершена успешно!"

    if [ "$BUILD_CPP_ONLY" = false ]; then
        echo
        print_status "Для установки APK:"
        echo "  adb install -r app/build/outputs/apk/$BUILD_TYPE/app-$BUILD_TYPE.apk"
    fi
}

main "$@"
