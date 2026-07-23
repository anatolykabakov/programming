#!/bin/bash

# Скрипт сборки только C++ части проекта ADAS (Conan + CMake)
# Автор: AI Assistant
# Версия: 1.1

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функция для вывода сообщений
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Переменные (пути относительно скрипта)
CPP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$CPP_DIR/../../../.." && pwd)"
BUILDOZER_PLATFORM="${BUILDOZER_PLATFORM:-}"

BUILD_DIR="build"
ABI="arm64-v8a"
PLATFORM="android-26"
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
BUILD_TARGET="android"  # android or linux
BUILD_TESTS=false       # build tests
# Filled after parse / before android build (app/libs/<abi>)
JNI_LIBS_DIR=""
# Conan runtime_deploy staging (like adcu_soc_sw CONAN_RUNTIME_DEPLOY_DIR)
CONAN_RUNTIME_DEPLOY_DIR=""

# Функция показа справки
show_help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo
    echo "ОПЦИИ:"
    echo "  -t, --target TARGET     Цель сборки (android|linux) [по умолчанию: android]"
    echo "  -a, --abi ABI          Архитектура (arm64-v8a|armeabi-v7a|x86|x86_64) [по умолчанию: arm64-v8a]"
    echo "  -p, --platform PLAT    Платформа Android (android-21|android-23|etc) [по умолчанию: android-26]"
    echo "  -b, --type TYPE         Тип сборки (Debug|Release) [по умолчанию: Release]"
    echo "  -c, --clean             Очистить перед сборкой"
    echo "  -v, --verbose           Подробный вывод"
    echo "  --test                  Собрать и запустить тесты (только для Linux)"
    echo "  -h, --help              Показать эту справку"
    echo
    echo "Env: ANDROID_NDK_ROOT / ANDROID_NDK_HOME, BUILDOZER_PLATFORM"
    echo
    echo "ПРИМЕРЫ:"
    echo "  $0                      # Сборка для Android arm64-v8a"
    echo "  $0 -t linux             # Сборка для Linux"
    echo "  $0 -t android -c -v     # Очистка и подробная сборка Android"
    echo "  $0 -t linux --test      # Сборка и запуск тестов для Linux"
}

# Парсинг аргументов
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--target)
                BUILD_TARGET="$2"
                shift 2
                ;;
            -a|--abi)
                ABI="$2"
                shift 2
                ;;
            -p|--platform)
                PLATFORM="$2"
                shift 2
                ;;
            -b|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            --test)
                BUILD_TESTS=true
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

resolve_ndk_path() {
    if [ -n "${ANDROID_NDK_ROOT:-}" ] && [ -d "$ANDROID_NDK_ROOT" ]; then
        NDK_PATH="$ANDROID_NDK_ROOT"
        return 0
    fi
    if [ -n "${ANDROID_NDK_HOME:-}" ] && [ -d "$ANDROID_NDK_HOME" ]; then
        NDK_PATH="$ANDROID_NDK_HOME"
        return 0
    fi
    if [ -n "${BUILDOZER_PLATFORM:-}" ] && [ -d "$BUILDOZER_PLATFORM/android-ndk-r28c" ]; then
        NDK_PATH="$BUILDOZER_PLATFORM/android-ndk-r28c"
        return 0
    fi
    if [ -f "$PROJECT_DIR/local.properties" ]; then
        local ndk
        ndk=$(grep -E '^ndk\.dir=' "$PROJECT_DIR/local.properties" | head -1 | cut -d= -f2- | tr -d '\r')
        if [ -n "$ndk" ] && [ -d "$ndk" ]; then
            NDK_PATH="$ndk"
            return 0
        fi
    fi
    return 1
}

# Проверка зависимостей
check_dependencies() {
    print_status "Проверка зависимостей для $BUILD_TARGET..."

    # Проверяем, что тесты можно собирать только для Linux
    if [ "$BUILD_TESTS" = true ] && [ "$BUILD_TARGET" != "linux" ]; then
        print_error "Тесты можно собирать только для Linux (--test работает только с -t linux)"
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        print_error "CMake не найден"
        exit 1
    fi

    if ! command -v conan &> /dev/null; then
        print_error "Conan не найден (pip install conan)"
        exit 1
    fi

    if [ ! -d "$CPP_DIR" ] || [ ! -f "$CPP_DIR/CMakeLists.txt" ]; then
        print_error "Директория C++ не найдена: $CPP_DIR"
        exit 1
    fi

    if [ "$BUILD_TARGET" = "android" ]; then
        if ! resolve_ndk_path; then
            print_error "Android NDK не найден (ANDROID_NDK_ROOT / buildozer / local.properties)"
            exit 1
        fi
        if [ ! -f "$NDK_PATH/build/cmake/android.toolchain.cmake" ]; then
            print_error "NDK toolchain не найден: $NDK_PATH"
            exit 1
        fi
        export ANDROID_NDK_ROOT="$NDK_PATH"
        export ANDROID_NDK_HOME="$NDK_PATH"
    elif [ "$BUILD_TARGET" = "linux" ]; then
        if [ "$BUILD_TESTS" = true ]; then
            if [ ! -f "$CPP_DIR/tests/CMakeLists.txt" ]; then
                print_error "CMakeLists.txt для тестов не найден: $CPP_DIR/tests/CMakeLists.txt"
                exit 1
            fi
        fi
    else
        print_error "Неизвестная цель сборки: $BUILD_TARGET"
        print_error "Поддерживаемые цели: android, linux"
        exit 1
    fi

    print_success "Все зависимости найдены"
}

# Очистка сборки
clean_build() {
    if [ "$CLEAN_BUILD" = true ]; then
        print_status "Очистка предыдущей сборки..."
        cd "$CPP_DIR"
        if [ -d "$BUILD_DIR" ]; then
            rm -rf "$BUILD_DIR"
            print_success "Директория сборки очищена"
        fi
    fi
}

# Map Android ABI → NDK triple for libc++_shared.so
android_ndk_triple() {
    case "$1" in
        arm64-v8a) echo "aarch64-linux-android" ;;
        armeabi-v7a) echo "arm-linux-androideabi" ;;
        x86) echo "i686-linux-android" ;;
        x86_64) echo "x86_64-linux-android" ;;
        *) return 1 ;;
    esac
}

conan_install() {
    local -a args=(-of "$CPP_DIR/$BUILD_DIR" --build=missing -s "build_type=$BUILD_TYPE")

    if [ "$BUILD_TARGET" = "android" ]; then
        local clang_ver=14
        if [ -x "$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" ]; then
            clang_ver=$("$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" --version | head -1 | sed -E 's/.*clang version ([0-9]+).*/\1/')
        fi
        # NDK r27 flags.cmake uses IN_LIST; zeromq cmake_minimum_required(3.0.x) leaves CMP0057 unset.
        # extra_variables alone is too late (after NDK include / before project()); push -D on configure
        # and a user_toolchain CACHE FORCE at the very start of conan_toolchain.cmake.
        local ndk_policy_tc="$CPP_DIR/cmake/ndk_cmp0057.cmake"
        CONAN_RUNTIME_DEPLOY_DIR="$CPP_DIR/$BUILD_DIR/runtime_deploy"
        # Deployers cannot overwrite symlinks left by a previous run (adcu_soc_sw/script/conan.sh).
        rm -rf "$CONAN_RUNTIME_DEPLOY_DIR"
        args+=(
            -s:h os=Android -s:h os.api_level=26 -s:h arch=armv8
            -s:h compiler=clang -s:h "compiler.version=$clang_ver" -s:h compiler.libcxx=c++_shared
            -s:h compiler.cppstd=17
            -c:h "tools.android:ndk_path=$NDK_PATH"
            -c:h "tools.cmake.cmaketoolchain:user_toolchain=[\"$ndk_policy_tc\"]"
            -c:h 'tools.cmake:configure_args=["-DCMAKE_POLICY_DEFAULT_CMP0057=NEW"]'
            --deployer=runtime_deploy
            --deployer-folder="$CONAN_RUNTIME_DEPLOY_DIR"
        )
    else
        args+=(-s:h os=Linux -s:h arch=x86_64)
        if [ "$BUILD_TESTS" = true ]; then
            args+=(-o "&:tests=True")
        fi
    fi

    print_status "Conan install ($BUILD_TARGET)..."
    (cd "$CPP_DIR" && conan install . "${args[@]}")
}

conan_toolchain() {
    if [ -f "$CPP_DIR/$BUILD_DIR/conan_toolchain.cmake" ]; then
        echo "$CPP_DIR/$BUILD_DIR/conan_toolchain.cmake"
    elif [ -f "$CPP_DIR/$BUILD_DIR/generators/conan_toolchain.cmake" ]; then
        echo "$CPP_DIR/$BUILD_DIR/generators/conan_toolchain.cmake"
    else
        print_error "conan_toolchain.cmake не найден в $CPP_DIR/$BUILD_DIR"
        exit 1
    fi
}

# Activate VirtualBuildEnv (puts build-context protoc/cmake on PATH).
conan_activate_build_env() {
    local env_sh="$CPP_DIR/$BUILD_DIR/conanbuild.sh"
    if [ ! -f "$env_sh" ]; then
        env_sh="$CPP_DIR/$BUILD_DIR/generators/conanbuild.sh"
    fi
    if [ -f "$env_sh" ]; then
        # shellcheck disable=SC1090
        source "$env_sh"
        print_status "Conan build env: $(command -v protoc || echo 'protoc not on PATH')"
        if command -v protoc >/dev/null 2>&1; then
            print_status "  protoc: $(protoc --version 2>/dev/null || true)"
        fi
    else
        print_warning "conanbuild.sh не найден — системный protoc может не совпасть с Conan protobuf"
    fi
}

# Сборка для Linux
build_linux() {
    print_status "Сборка для Linux..."

    cd "$CPP_DIR"
    mkdir -p "$BUILD_DIR"

    conan_install
    local toolchain
    toolchain=$(conan_toolchain)
    conan_activate_build_env

    print_status "Конфигурация CMake для Linux..."
    print_status "  - Build Type: $BUILD_TYPE"
    print_status "  - Toolchain: $toolchain"

    local -a cmake_args=(
        -DCMAKE_TOOLCHAIN_FILE="$toolchain"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DBUILD_FOR_ANDROID=OFF
        -DCMAKE_PREFIX_PATH="$CPP_DIR/$BUILD_DIR"
        -UProtobuf_PROTOC_EXECUTABLE
        -UPROTOC_PROGRAM
    )
    if command -v protoc >/dev/null 2>&1; then
        cmake_args+=(-DProtobuf_PROTOC_EXECUTABLE="$(command -v protoc)")
    fi
    if [ "$BUILD_TESTS" = true ]; then
        cmake_args+=(-DBUILD_TESTING=ON)
        print_status "  - Build Testing: ON"
    else
        cmake_args+=(-DBUILD_TESTING=OFF)
        print_status "  - Build Testing: OFF"
    fi

    cmake "${cmake_args[@]}" -B "$BUILD_DIR" -S .

    print_status "Компиляция C++ кода для Linux..."
    if [ "$VERBOSE" = true ]; then
        cmake --build "$BUILD_DIR" --verbose
    else
        cmake --build "$BUILD_DIR"
    fi

    if [ "$BUILD_TESTS" = true ]; then
        echo
        print_status "Запуск тестов..."
        TEST_EXECUTABLE="$BUILD_DIR/tests/adas_tests"
        if [ -f "$TEST_EXECUTABLE" ]; then
            cd "$BUILD_DIR"
            if ./tests/adas_tests; then
                print_success "Все тесты прошли успешно!"
            else
                exit_code=$?
                print_error "Некоторые тесты не прошли (код: $exit_code)"
                exit 1
            fi
            cd "$CPP_DIR"
        else
            print_error "Тестовый исполняемый файл не найден: $TEST_EXECUTABLE"
            exit 1
        fi
    fi
}

# Сборка для Android
build_android() {
    print_status "Сборка для Android..."

    cd "$CPP_DIR"
    mkdir -p "$BUILD_DIR"

    print_status "Конфигурация CMake..."
    print_status "  - ABI: $ABI"
    print_status "  - Platform: $PLATFORM"
    print_status "  - Build Type: $BUILD_TYPE"
    print_status "  - NDK: $NDK_PATH"

    export ANDROID_NDK_HOME="$NDK_PATH"
    export ANDROID_ABI="$ABI"
    export ANDROID_PLATFORM="$PLATFORM"

    conan_install
    local toolchain
    toolchain=$(conan_toolchain)
    conan_activate_build_env

    local -a cmake_args=(
        -DCMAKE_TOOLCHAIN_FILE="$toolchain"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DANDROID_ABI="$ABI"
        -DANDROID_PLATFORM="$PLATFORM"
        -DBUILD_FOR_ANDROID=ON
        -DCMAKE_PREFIX_PATH="$CPP_DIR/$BUILD_DIR"
        -UProtobuf_PROTOC_EXECUTABLE
        -UPROTOC_PROGRAM
    )
    if command -v protoc >/dev/null 2>&1; then
        cmake_args+=(-DProtobuf_PROTOC_EXECUTABLE="$(command -v protoc)")
    fi

    cmake "${cmake_args[@]}" -B "$CPP_DIR/$BUILD_DIR" -S "$CPP_DIR"

    print_status "Компиляция C++ кода..."
    if [ "$VERBOSE" = true ]; then
        cmake --build "$CPP_DIR/$BUILD_DIR" --verbose
    else
        cmake --build "$CPP_DIR/$BUILD_DIR"
    fi

    LIBRARY_PATH="$CPP_DIR/$BUILD_DIR/libadas_app_android.so"
    if [ ! -f "$LIBRARY_PATH" ]; then
        LIBRARY_PATH=$(find "$CPP_DIR/$BUILD_DIR" -name 'libadas_app_android.so' -type f | head -1 || true)
    fi

    if [ -n "$LIBRARY_PATH" ] && [ -f "$LIBRARY_PATH" ]; then
        BUILT_LIBRARY="$LIBRARY_PATH"
        print_success "C++ библиотека создана: $LIBRARY_PATH"
        echo
        print_status "Информация о библиотеке:"
        ls -la "$LIBRARY_PATH"
        echo
        print_status "Размер: $(du -h "$LIBRARY_PATH" | cut -f1)"
        print_status "Архитектура: $ABI"
        print_status "Платформа: $PLATFORM"

        if command -v readelf &> /dev/null; then
            echo
            print_status "Зависимости библиотеки:"
            readelf -d "$LIBRARY_PATH" | grep NEEDED || echo "  Нет зависимостей"
        fi
    else
        print_error "Ошибка: C++ библиотека не создана"
        return 1
    fi
}

# Выбор функции сборки
build_cpp_library() {
    if [ "$BUILD_TARGET" = "android" ]; then
        build_android
    elif [ "$BUILD_TARGET" = "linux" ]; then
        build_linux
    else
        print_error "Неизвестная цель сборки: $BUILD_TARGET"
        exit 1
    fi
}

# Copy Conan runtime_deploy shared libs + NDK libc++_shared + app .so → app/libs/<abi>
copy_to_jnilibs() {
    if [ "$BUILD_TARGET" != "android" ]; then
        print_status "Копирование в jniLibs пропущено (не Android сборка)"
        return 0
    fi

    JNI_LIBS_DIR="$PROJECT_DIR/app/libs/$ABI"
    CONAN_RUNTIME_DEPLOY_DIR="${CONAN_RUNTIME_DEPLOY_DIR:-$CPP_DIR/$BUILD_DIR/runtime_deploy}"

    print_status "Копирование библиотек в jniLibs ($ABI)..."
    rm -rf "$JNI_LIBS_DIR"
    mkdir -p "$JNI_LIBS_DIR"

    local copied=0
    if [ -d "$CONAN_RUNTIME_DEPLOY_DIR" ]; then
        # Flatten unversioned lib*.so only (Android loads by exact name; cp -L resolves symlinks).
        local f base
        for f in "$CONAN_RUNTIME_DEPLOY_DIR"/lib*.so; do
            [ -e "$f" ] || continue
            base=$(basename "$f")
            case "$base" in
                *.so.*) continue ;;  # skip libfoo.so.1 style
                libprotoc.so) continue ;;  # host tool, not needed at runtime
            esac
            cp -L "$f" "$JNI_LIBS_DIR/$base"
            copied=$((copied + 1))
        done
        print_status "  Conan runtime_deploy: $copied shared lib(s) → $JNI_LIBS_DIR"
    else
        print_warning "runtime_deploy не найден: $CONAN_RUNTIME_DEPLOY_DIR"
    fi

    local triple
    if ! triple=$(android_ndk_triple "$ABI"); then
        print_error "Неизвестный ABI для libc++_shared: $ABI"
        exit 1
    fi
    local cxx_shared="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$triple/libc++_shared.so"
    if [ ! -f "$cxx_shared" ]; then
        print_error "libc++_shared.so не найден: $cxx_shared"
        exit 1
    fi
    cp -f "$cxx_shared" "$JNI_LIBS_DIR/libc++_shared.so"

    if [ -z "${BUILT_LIBRARY:-}" ] || [ ! -f "$BUILT_LIBRARY" ]; then
        print_error "Собранная библиотека не найдена (BUILT_LIBRARY)"
        exit 1
    fi
    cp -f "$BUILT_LIBRARY" "$JNI_LIBS_DIR/libadas_app_android.so"

    print_success "jniLibs готовы → $JNI_LIBS_DIR"
    ls -la "$JNI_LIBS_DIR"
}

# Показать информацию о сборке
show_build_info() {
    echo
    print_status "Информация о сборке C++:"
    echo "  - Target: $BUILD_TARGET"
    if [ "$BUILD_TARGET" = "android" ]; then
        echo "  - ABI: $ABI"
        echo "  - Platform: $PLATFORM"
        echo "  - NDK: ${NDK_PATH:-"(resolve later)"}"
    fi
    echo "  - Build Type: $BUILD_TYPE"
    echo "  - Clean Build: $CLEAN_BUILD"
    echo "  - Verbose: $VERBOSE"
    echo "  - Build Dir: $CPP_DIR/$BUILD_DIR"
    echo "  - Deps: Conan (conanfile.py)"
    if [ "$BUILD_TARGET" = "android" ]; then
        echo "  - jniLibs: $PROJECT_DIR/app/libs/$ABI"
        echo "  - runtime_deploy: $CPP_DIR/$BUILD_DIR/runtime_deploy"
    fi
    echo
}

# Основная функция
main() {
    echo "=========================================="
    echo "  Сборка C++ части проекта ADAS (Conan)"
    echo "=========================================="
    echo

    BUILT_LIBRARY=""
    parse_arguments "$@"
    if [ "$BUILD_TARGET" = "android" ]; then
        BUILD_DIR="build-android"
    else
        BUILD_DIR="build-linux"
    fi
    show_build_info

    check_dependencies
    clean_build
    build_cpp_library
    copy_to_jnilibs

    echo
    print_success "Сборка C++ части завершена успешно!"
    if [ "$BUILD_TARGET" = "android" ]; then
        print_status "Библиотека готова для использования в Android приложении"
    else
        print_status "Linux библиотека готова для использования"
    fi
}

# Запуск скрипта
main "$@"
