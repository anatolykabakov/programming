#!/bin/bash

# Скрипт сборки только C++ части проекта ADAS
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
CPP_DIR="/workspace/programming/android/adas/app/src/main/cpp"
NDK_PATH="/workspace/Android/Sdk/ndk/27.0.12077973"
ANDROID_TOOLCHAIN_FILE=$NDK_PATH/build/cmake/android.toolchain.cmake

BUILD_DIR="build"
ABI="arm64-v8a"
PLATFORM="android-26"
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
BUILD_TARGET="android"  # android or linux
BUILD_TESTS=false       # build tests

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
    echo "ПРИМЕРЫ:"
    echo "  $0                      # Сборка для Android arm64-v8a"
    echo "  $0 -t linux             # Сборка для Linux"
    echo "  $0 -t android -a armeabi-v7a  # Сборка для Android 32-bit ARM"
    echo "  $0 -t linux -c -v       # Очистка и подробная сборка для Linux"
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

# Проверка зависимостей
check_dependencies() {
    print_status "Проверка зависимостей для $BUILD_TARGET..."

    # Проверяем, что тесты можно собирать только для Linux
    if [ "$BUILD_TESTS" = true ] && [ "$BUILD_TARGET" != "linux" ]; then
        print_error "Тесты можно собирать только для Linux (--test работает только с -t linux)"
        exit 1
    fi

    # Проверяем CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake не найден"
        exit 1
    fi

    # Проверяем директорию C++
    if [ ! -d "$CPP_DIR" ]; then
        print_error "Директория C++ не найдена: $CPP_DIR"
        exit 1
    fi

    # Проверяем зависимости в зависимости от цели сборки
    if [ "$BUILD_TARGET" = "android" ]; then
        # Проверяем NDK для Android
        if [ ! -d "$NDK_PATH" ]; then
            print_error "Android NDK не найден в $NDK_PATH"
            print_error "Запустите install_dependencies.sh"
            exit 1
        fi

        # Проверяем vcpkg для Android
        if [ ! -d "/workspace/vcpkg" ]; then
            print_error "vcpkg не найден в /workspace/vcpkg"
            print_error "Запустите install_dependencies.sh"
            exit 1
        fi
    elif [ "$BUILD_TARGET" = "linux" ]; then
        # Проверяем vcpkg для Linux
        if [ ! -d "/workspace/vcpkg" ]; then
            print_error "vcpkg не найден в /workspace/vcpkg"
            print_error "Запустите install_dependencies.sh"
            exit 1
        fi

        # Проверяем CMake
        if ! command -v cmake &> /dev/null; then
            print_error "CMake не найден"
            exit 1
        fi

        # Проверяем зависимости для тестов
        if [ "$BUILD_TESTS" = true ]; then
            print_status "Проверка зависимостей для тестов..."

            # Проверяем директорию тестов
            if [ ! -d "$CPP_DIR/tests" ]; then
                print_error "Директория тестов не найдена: $CPP_DIR/tests"
                exit 1
            fi

            # Проверяем CMakeLists.txt для тестов
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

# Сборка для Linux
build_linux() {
    print_status "Сборка для Linux..."

    cd "$CPP_DIR"

    # Создаем директорию сборки
    mkdir -p "$BUILD_DIR"

    print_status "Конфигурация CMake для Linux..."
    print_status "  - Build Type: $BUILD_TYPE"
    print_status "  - Target: Linux"
    print_status "  - vcpkg: /workspace/vcpkg"

    # Устанавливаем переменные окружения для vcpkg
    export VCPKG_ROOT="/workspace/vcpkg"
    export ADAS_VCPKG_DIR="/workspace/vcpkg"

    # Конфигурируем CMake для Linux с vcpkg
    CMAKE_ARGS=(
        -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
        -DVCPKG_TARGET_TRIPLET=x64-linux
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE
        -DBUILD_FOR_ANDROID=OFF
    )

    # Добавляем BUILD_TESTING если нужно собирать тесты
    if [ "$BUILD_TESTS" = true ]; then
        CMAKE_ARGS+=(-DBUILD_TESTING=ON)
        CMAKE_ARGS+=(-DVCPKG_MANIFEST_FEATURES=tests)
        print_status "  - Build Testing: ON"
        print_status "  - Vcpkg Features: tests"
    else
        CMAKE_ARGS+=(-DBUILD_TESTING=OFF)
        print_status "  - Build Testing: OFF"
    fi

    cmake "${CMAKE_ARGS[@]}" -B "$BUILD_DIR" -S .

    # Собираем
    print_status "Компиляция C++ кода для Linux..."
    if [ "$VERBOSE" = true ]; then
        cmake --build "$BUILD_DIR" --verbose
    else
        cmake --build "$BUILD_DIR"
    fi

    # Запускаем тесты если нужно
    if [ "$BUILD_TESTS" = true ]; then
        echo
        print_status "Запуск тестов..."

        # Проверяем, что тесты собрались
        TEST_EXECUTABLE="$BUILD_DIR/tests/adas_tests"
        if [ -f "$TEST_EXECUTABLE" ]; then
            print_status "Тестовый исполняемый файл найден: $TEST_EXECUTABLE"

            # Запускаем тесты
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

    # Создаем директорию сборки
    mkdir -p "$BUILD_DIR"

    print_status "Конфигурация CMake..."
    print_status "  - ABI: $ABI"
    print_status "  - Platform: $PLATFORM"
    print_status "  - Build Type: $BUILD_TYPE"
    print_status "  - NDK: $NDK_PATH"

    # Конфигурируем CMake с корневого уровня
    print_status "Конфигурация CMake с корневого уровня..."
    cd "/workspace/programming/android/adas"

    # Устанавливаем переменные окружения для Android
    export ANDROID_NDK_HOME="$NDK_PATH"
    export ANDROID_ABI="$ABI"
    export ANDROID_PLATFORM="$PLATFORM"
    export ADAS_VCPKG_DIR="/workspace/vcpkg"
    export VCPKG_ROOT="/workspace/vcpkg"

    # Android Studio build command
    #   /usr/bin/cmake \
    #     -H/workspace/programming/android/adas/app/src/main/cpp \
    #     -DCMAKE_SYSTEM_NAME=Android \
    #     -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    #     -DCMAKE_SYSTEM_VERSION=24 \
    #     -DANDROID_PLATFORM=android-24 \
    #     -DANDROID_ABI=arm64-v8a \
    #     -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    #     -DANDROID_NDK=/opt/android-ndk-r25b \
    #     -DCMAKE_ANDROID_NDK=/opt/android-ndk-r25b \
    #     -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk-r25b/build/cmake/android.toolchain.cmake \
    #     -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
    #     -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=/workspace/programming/android/adas/app/build/intermediates/cxx/RelWithDebInfo/342b6066/obj/arm64-v8a \
    #     -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/workspace/programming/android/adas/app/build/intermediates/cxx/RelWithDebInfo/342b6066/obj/arm64-v8a \
    #     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    #     -B/workspace/programming/android/adas/app/.cxx/RelWithDebInfo/342b6066/arm64-v8a \
    #     -GNinja

    cmake \
        -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$NDK_PATH/build/cmake/android.toolchain.cmake \
        -DVCPKG_TARGET_TRIPLET=arm64-android \
        -DANDROID_ABI=$ABI \
        -B "$CPP_DIR/$BUILD_DIR" -S $CPP_DIR

    # Собираем
    print_status "Компиляция C++ кода..."
    if [ "$VERBOSE" = true ]; then
        cmake --build "$CPP_DIR/$BUILD_DIR" --verbose
    else
        cmake --build "$CPP_DIR/$BUILD_DIR"
    fi

    # Проверяем результат
    LIBRARY_PATH="$CPP_DIR/$BUILD_DIR/libadas_app_android.so"
    if [ -f "$LIBRARY_PATH" ]; then
        print_success "C++ библиотека создана: $LIBRARY_PATH"

        # Показываем информацию о библиотеке
        echo
        print_status "Информация о библиотеке:"
        ls -la "$LIBRARY_PATH"
        echo
        print_status "Размер: $(du -h "$LIBRARY_PATH" | cut -f1)"
        print_status "Архитектура: $ABI"
        print_status "Платформа: $PLATFORM"

        # Проверяем зависимости (если доступно)
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

# Копирование библиотеки в jniLibs (только для Android)
copy_to_jnilibs() {
    if [ "$BUILD_TARGET" = "android" ]; then
        print_status "Копирование библиотек в jniLibs..."

        JNI_LIBS_DIR="/workspace/programming/android/adas/app/libs/arm64-v8a"
        mkdir -p "$JNI_LIBS_DIR"

        # Копируем основную библиотеку
        cp "$CPP_DIR/$BUILD_DIR/libadas_app_android.so" "$JNI_LIBS_DIR/"
        print_success "✓ libadas_app_android.so скопирована"

        print_success "Все библиотеки скопированы в $JNI_LIBS_DIR"
    else
        print_status "Копирование в jniLibs пропущено (не Android сборка)"
    fi
}

# Показать информацию о сборке
show_build_info() {
    echo
    print_status "Информация о сборке C++:"
    echo "  - Target: $BUILD_TARGET"
    if [ "$BUILD_TARGET" = "android" ]; then
        echo "  - ABI: $ABI"
        echo "  - Platform: $PLATFORM"
    fi
    echo "  - Build Type: $BUILD_TYPE"
    echo "  - Clean Build: $CLEAN_BUILD"
    echo "  - Verbose: $VERBOSE"
    echo "  - Build Dir: $BUILD_DIR"
    echo
}

# Основная функция
main() {
    echo "=========================================="
    echo "  Сборка C++ части проекта ADAS"
    echo "=========================================="
    echo

    parse_arguments "$@"
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
