#!/bin/bash

# Скрипт установки зависимостей для проекта ADAS

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NDK_VERSION="27.0.12077973"
# Prefer existing env / buildozer NDK; otherwise install under $HOME/Android/Sdk
DEFAULT_SDK_HOME="${ANDROID_HOME:-/usr/lib/android-sdk}"
DEFAULT_NDK_HOME="${HOME}/Android/Sdk/ndk/${NDK_VERSION}"
BUILDOZER_NDK="${BUILDOZER_NDK:-}"

print_status()  { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }

resolve_ndk() {
    if [ -n "${ANDROID_NDK_ROOT:-}" ] && [ -d "$ANDROID_NDK_ROOT" ]; then
        echo "$ANDROID_NDK_ROOT"
        return
    fi
    if [ -n "${ANDROID_NDK_HOME:-}" ] && [ -d "$ANDROID_NDK_HOME" ]; then
        echo "$ANDROID_NDK_HOME"
        return
    fi
    if [ -d "$BUILDOZER_NDK" ]; then
        echo "$BUILDOZER_NDK"
        return
    fi
    if [ -d "$DEFAULT_NDK_HOME" ]; then
        echo "$DEFAULT_NDK_HOME"
        return
    fi
    # any ndk under home Android Sdk
    local found
    found="$(ls -d "${HOME}/Android/Sdk/ndk"/* 2>/dev/null | head -1 || true)"
    if [ -n "$found" ] && [ -d "$found" ]; then
        echo "$found"
        return
    fi
    echo ""
}

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

update_system() {
    print_status "Обновление системы..."
    sudo apt update && sudo apt upgrade -y
    print_success "Система обновлена"
}

install_java() {
    print_status "Установка Java 21 OpenJDK..."
    sudo apt install -y openjdk-21-jdk
    export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
    if ! grep -q 'JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64' ~/.bashrc 2>/dev/null; then
        echo 'export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64' >> ~/.bashrc
    fi
    print_success "Java 21 установлена"
}

install_android_sdk() {
    print_status "Установка Android SDK..."
    sudo apt install -y android-sdk android-sdk-platform-tools android-sdk-build-tools

    ANDROID_HOME="$DEFAULT_SDK_HOME"
    export ANDROID_HOME
    if ! grep -q "ANDROID_HOME=${ANDROID_HOME}" ~/.bashrc 2>/dev/null; then
        echo "export ANDROID_HOME=${ANDROID_HOME}" >> ~/.bashrc
    fi

    print_status "Принятие лицензий Android SDK..."
    sudo mkdir -p "$ANDROID_HOME/licenses"
    echo "24333f8a63b6825ea9c5514f83c2829b004d1fee" | sudo tee "$ANDROID_HOME/licenses/android-sdk-license" >/dev/null
    echo "d56f5187479451eabf01fb78e6d74f98c2a2628b" | sudo tee "$ANDROID_HOME/licenses/android-sdk-preview-license" >/dev/null

    print_success "Android SDK: $ANDROID_HOME"
}

install_android_ndk() {
    print_status "Проверка Android NDK..."

    local existing
    existing="$(resolve_ndk)"
    if [ -n "$existing" ]; then
        export ANDROID_NDK_ROOT="$existing"
        print_success "Android NDK уже есть: $ANDROID_NDK_ROOT"
        if ! grep -q "ANDROID_NDK_ROOT=" ~/.bashrc 2>/dev/null; then
            echo "export ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT}" >> ~/.bashrc
        fi
        return 0
    fi

    print_status "Установка Android NDK ${NDK_VERSION} → ${DEFAULT_NDK_HOME}"
    mkdir -p "$(dirname "$DEFAULT_NDK_HOME")"
    local tmpdir
    tmpdir="$(mktemp -d)"
    pushd "$tmpdir" >/dev/null

    if [ ! -f android-ndk-r27-linux.zip ]; then
        print_status "Скачивание Android NDK..."
        wget -q --show-progress https://dl.google.com/android/repository/android-ndk-r27-linux.zip
    fi
    print_status "Распаковка Android NDK..."
    unzip -q android-ndk-r27-linux.zip
    # archive extracts to android-ndk-r27
    rm -rf "$DEFAULT_NDK_HOME"
    mv android-ndk-r27 "$DEFAULT_NDK_HOME"
    popd >/dev/null
    rm -rf "$tmpdir"

    export ANDROID_NDK_ROOT="$DEFAULT_NDK_HOME"
    if ! grep -q "ANDROID_NDK_ROOT=" ~/.bashrc 2>/dev/null; then
        echo "export ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT}" >> ~/.bashrc
    fi
    print_success "Android NDK установлен: $ANDROID_NDK_ROOT"
}

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

create_local_properties() {
    print_status "Создание файла local.properties..."

    local sdk_dir="${ANDROID_HOME:-$DEFAULT_SDK_HOME}"
    local ndk_dir
    ndk_dir="$(resolve_ndk)"
    if [ -z "$ndk_dir" ]; then
        ndk_dir="${ANDROID_NDK_ROOT:-$DEFAULT_NDK_HOME}"
    fi

    # Gradle wants escaped paths on Windows; on Linux plain absolute paths are fine
    cat > "${PROJECT_DIR}/local.properties" << EOF
sdk.dir=${sdk_dir}
ndk.dir=${ndk_dir}
EOF

    print_success "local.properties → ${PROJECT_DIR}/local.properties"
    print_status "  sdk.dir=${sdk_dir}"
    print_status "  ndk.dir=${ndk_dir}"
}

verify_installation() {
    print_status "Проверка установки..."

    if command -v java &> /dev/null; then
        print_success "Java: $(java -version 2>&1 | head -n 1)"
    else
        print_error "Java не найдена"
        return 1
    fi

    local sdk_dir="${ANDROID_HOME:-$DEFAULT_SDK_HOME}"
    if [ -d "$sdk_dir" ]; then
        print_success "Android SDK: $sdk_dir"
    else
        print_error "Android SDK не найден: $sdk_dir"
        return 1
    fi

    local ndk_dir
    ndk_dir="$(resolve_ndk)"
    if [ -n "$ndk_dir" ]; then
        print_success "Android NDK: $ndk_dir"
    else
        print_error "Android NDK не найден"
        return 1
    fi

    if command -v cmake &> /dev/null; then
        print_success "CMake: $(cmake --version | head -n 1)"
    else
        print_error "CMake не найден"
        return 1
    fi

    if [ -f "${PROJECT_DIR}/local.properties" ]; then
        print_success "local.properties OK"
    else
        print_error "local.properties отсутствует"
        return 1
    fi

    print_success "Все зависимости установлены успешно!"
}

main() {
    echo "=========================================="
    echo "  Установка зависимостей для проекта ADAS"
    echo "  PROJECT_DIR=${PROJECT_DIR}"
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
    print_status "Для применения изменений: source ~/.bashrc"
}

# Allow: ./install_dependencies.sh --local-properties-only
if [[ "${1:-}" == "--local-properties-only" ]]; then
    create_local_properties
    exit 0
fi

main "$@"
