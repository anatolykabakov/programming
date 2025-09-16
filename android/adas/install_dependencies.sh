#!/bin/bash

# Скрипт установки зависимостей для проекта ADAS
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

# Проверка прав root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_warning "Скрипт запущен с правами root. Это может быть небезопасно."
        read -p "Продолжить? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# Обновление системы
update_system() {
    print_status "Обновление системы..."
    sudo apt update && sudo apt upgrade -y
    print_success "Система обновлена"
}

# Установка Java
install_java() {
    print_status "Установка Java 21 OpenJDK..."
    sudo apt install -y openjdk-21-jdk
    export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
    echo 'export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64' >> ~/.bashrc
    print_success "Java 21 установлена"
}

# Установка Android SDK
install_android_sdk() {
    print_status "Установка Android SDK..."
    sudo apt install -y android-sdk android-sdk-platform-tools android-sdk-build-tools
    
    # Создаем директорию для Android SDK
    ANDROID_HOME="/usr/lib/android-sdk"
    export ANDROID_HOME
    echo 'export ANDROID_HOME=/usr/lib/android-sdk' >> ~/.bashrc
    
    # Принимаем лицензии
    print_status "Принятие лицензий Android SDK..."
    sudo mkdir -p $ANDROID_HOME/licenses
    echo "24333f8a63b6825ea9c5514f83c2829b004d1fee" | sudo tee $ANDROID_HOME/licenses/android-sdk-license
    echo "d56f5187479451eabf01fb78e6d74f98c2a2628b" | sudo tee $ANDROID_HOME/licenses/android-sdk-preview-license
    
    print_success "Android SDK установлен"
}

# Установка Android NDK
install_android_ndk() {
    print_status "Проверка Android NDK..."
    
    # Проверяем, есть ли уже установленный NDK
    if [ -d "/workspace/Android/Sdk/ndk/27.0.12077973" ]; then
        print_success "Android NDK 27.0.12077973 уже установлен"
        export ANDROID_NDK_ROOT=/workspace/Android/Sdk/ndk/27.0.12077973
        echo 'export ANDROID_NDK_ROOT=/workspace/Android/Sdk/ndk/27.0.12077973' >> ~/.bashrc
        return 0
    fi
    
    print_status "Установка Android NDK 27.0.12077973..."
    
    # Скачиваем NDK
    if [ ! -f "android-ndk-r27-linux.zip" ]; then
        print_status "Скачивание Android NDK..."
        wget https://dl.google.com/android/repository/android-ndk-r27-linux.zip
    fi
    
    # Создаем директорию для Android SDK
    sudo mkdir -p /workspace/Android/Sdk/ndk
    
    # Распаковываем NDK
    if [ ! -d "android-ndk-r27" ]; then
        print_status "Распаковка Android NDK..."
        unzip android-ndk-r27-linux.zip
    fi
    
    # Перемещаем в нужную директорию
    sudo mv android-ndk-r27 /workspace/Android/Sdk/ndk/27.0.12077973
    export ANDROID_NDK_ROOT=/workspace/Android/Sdk/ndk/27.0.12077973
    echo 'export ANDROID_NDK_ROOT=/workspace/Android/Sdk/ndk/27.0.12077973' >> ~/.bashrc
    
    # Очищаем временные файлы
    rm -f android-ndk-r27-linux.zip
    
    print_success "Android NDK установлен"
}

# Установка дополнительных инструментов
install_tools() {
    print_status "Установка дополнительных инструментов..."
    sudo apt install -y \
        build-essential \
        cmake \
        ninja-build \
        git \
        wget \
        unzip \
        curl \
        pkg-config \
        libprotobuf-dev \
        protobuf-compiler
    
    print_success "Дополнительные инструменты установлены"
}

# Создание файла local.properties
create_local_properties() {
    print_status "Создание файла local.properties..."
    
    cat > /workspace/programming/android/adas/local.properties << EOF
sdk.dir=/workspace/Android/Sdk
ndk.dir=/workspace/Android/Sdk/ndk/27.0.12077973
EOF
    
    print_success "Файл local.properties создан"
}

# Проверка установки
verify_installation() {
    print_status "Проверка установки..."
    
    # Проверяем Java
    if command -v java &> /dev/null; then
        java_version=$(java -version 2>&1 | head -n 1)
        print_success "Java: $java_version"
    else
        print_error "Java не найдена"
        return 1
    fi
    
    # Проверяем Android SDK
    if [ -d "/usr/lib/android-sdk" ]; then
        print_success "Android SDK установлен"
    else
        print_error "Android SDK не найден"
        return 1
    fi
    
    # Проверяем Android NDK
    if [ -d "/workspace/Android/Sdk/ndk/27.0.12077973" ]; then
        print_success "Android NDK установлен"
    else
        print_error "Android NDK не найден"
        return 1
    fi
    
    # Проверяем CMake
    if command -v cmake &> /dev/null; then
        cmake_version=$(cmake --version | head -n 1)
        print_success "CMake: $cmake_version"
    else
        print_error "CMake не найден"
        return 1
    fi
    
    print_success "Все зависимости установлены успешно!"
}

# Основная функция
main() {
    echo "=========================================="
    echo "  Установка зависимостей для проекта ADAS"
    echo "=========================================="
    echo
    
    check_root
    
    print_status "Начинаем установку зависимостей..."
    
    update_system
    install_java
    install_android_sdk
    install_android_ndk
    install_tools
    create_local_properties
    verify_installation
    
    echo
    print_success "Установка завершена успешно!"
    print_status "Для применения изменений выполните: source ~/.bashrc"
    print_status "Или перезапустите терминал"
}

# Запуск скрипта
main "$@"





