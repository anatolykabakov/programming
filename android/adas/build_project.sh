#!/bin/bash

# Скрипт сборки проекта ADAS
# Автор: AI Assistant
# Версия: 1.0

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

# Переменные
PROJECT_DIR="/workspace/programming/android/adas"
BUILD_TYPE="debug"
CLEAN_BUILD=false
BUILD_CPP_ONLY=false
VERBOSE=false

# Функция показа справки
show_help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo
    echo "ОПЦИИ:"
    echo "  -t, --type TYPE        Тип сборки (debug|release) [по умолчанию: debug]"
    echo "  -c, --clean            Очистить перед сборкой"
    echo "  --cpp-only             Собрать только C++ часть"
    echo "  -v, --verbose          Подробный вывод"
    echo "  -h, --help             Показать эту справку"
    echo
    echo "ПРИМЕРЫ:"
    echo "  $0                     # Сборка debug версии"
    echo "  $0 -t release -c       # Очистка и сборка release версии"
    echo "  $0 --cpp-only          # Сборка только C++ части"
}

# Парсинг аргументов
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

# Проверка зависимостей
check_dependencies() {
    print_status "Проверка зависимостей..."
    
    # Проверяем Java
    if ! command -v java &> /dev/null; then
        print_error "Java не найдена. Запустите install_dependencies.sh"
        exit 1
    fi
    
    # Проверяем Android SDK
    if [ ! -d "/usr/lib/android-sdk" ]; then
        print_error "Android SDK не найден. Запустите install_dependencies.sh"
        exit 1
    fi
    
    # Проверяем Android NDK
    if [ ! -d "/workspace/Android/Sdk/ndk/27.0.12077973" ]; then
        print_error "Android NDK не найден. Запустите install_dependencies.sh"
        exit 1
    fi
    
    # Проверяем Gradle
    if [ ! -f "$PROJECT_DIR/gradlew" ]; then
        print_error "Gradle wrapper не найден в $PROJECT_DIR"
        exit 1
    fi
    
    print_success "Все зависимости найдены"
}

# Настройка переменных окружения
setup_environment() {
    print_status "Настройка переменных окружения..."
    
    export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
    export ANDROID_HOME=/usr/lib/android-sdk
    export ANDROID_NDK_ROOT=/workspace/Android/Sdk/ndk/27.0.12077973
    export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools
    
    print_success "Переменные окружения настроены"
}

# Очистка проекта
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
    
    cd app/src/main/cpp
    ./build_cpp.sh -c
    cd -
}


# Сборка Android приложения
build_android() {
    print_status "Сборка Android приложения..."
    
    cd "$PROJECT_DIR"
    
    # Определяем тип сборки
    if [ "$BUILD_TYPE" = "release" ]; then
        GRADLE_TASK="assembleRelease"
    else
        GRADLE_TASK="assembleDebug"
    fi
    
    # Собираем
    print_status "Выполнение: ./gradlew $GRADLE_TASK"
    if [ "$VERBOSE" = true ]; then
        ./gradlew "$GRADLE_TASK" --info
    else
        ./gradlew "$GRADLE_TASK"
    fi
    
    # Проверяем результат
    APK_DIR="app/build/outputs/apk/$BUILD_TYPE"
    if [ -d "$APK_DIR" ] && [ "$(ls -A $APK_DIR)" ]; then
        print_success "APK создан в: $APK_DIR"
        ls -la "$APK_DIR"/*.apk
    else
        print_error "Ошибка: APK не создан"
        return 1
    fi
}

# Показать информацию о сборке
show_build_info() {
    echo
    print_status "Информация о сборке:"
    echo "  - Тип сборки: $BUILD_TYPE"
    echo "  - Очистка: $CLEAN_BUILD"
    echo "  - Только C++: $BUILD_CPP_ONLY"
    echo "  - Подробный вывод: $VERBOSE"
    echo
}

# Основная функция
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
    build_android
    
    echo
    print_success "Сборка завершена успешно!"
    
    if [ "$BUILD_CPP_ONLY" = false ]; then
        echo
        print_status "Для установки APK на устройство используйте:"
        echo "  adb install app/build/outputs/apk/$BUILD_TYPE/app-$BUILD_TYPE.apk"
    fi
}

# Запуск скрипта
main "$@"















