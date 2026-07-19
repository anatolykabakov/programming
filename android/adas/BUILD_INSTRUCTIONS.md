# Инструкции по сборке проекта ADAS

## Обзор

Проект ADAS (Advanced Driver Assistance System) - это Android приложение с нативным C++ кодом для обработки данных с датчиков автомобиля.

## Структура проекта

```
adas/
├── app/
│   ├── src/main/
│   │   ├── cpp/                    # C++ исходный код
│   │   │   ├── zmq_reader.cpp      # Основной модуль
│   │   │   ├── zmq_config.cpp      # Конфигурация ZMQ
│   │   │   ├── json_config.cpp     # Конфигурация JSON
│   │   │   └── protobuf_utils.cpp  # Утилиты Protobuf
│   │   ├── java/                   # Java код Android
│   │   └── jniLibs/               # Нативные библиотеки
│   └── build.gradle               # Конфигурация сборки
├── install_dependencies.sh        # Скрипт установки зависимостей
├── build_project.sh              # Скрипт сборки проекта
├── build_cpp_only.sh             # Скрипт сборки только C++
└── local.properties              # Локальные настройки
```

## Быстрый старт

### 1. Установка зависимостей

```bash
# Сделать скрипты исполняемыми
chmod +x *.sh

# Установить все зависимости
./install_dependencies.sh
```

### 2. Сборка проекта

```bash
# Сборка debug версии
./build_project.sh

# Сборка release версии
./build_project.sh -t release

# Очистка и сборка
./build_project.sh -c -t release
```

### 3. Сборка только C++ части

```bash
# Сборка для arm64-v8a
./build_cpp_only.sh

# Сборка для 32-bit ARM
./build_cpp_only.sh -a armeabi-v7a

# Подробная сборка
./build_cpp_only.sh -v
```

## Детальные инструкции

### Установка зависимостей

Скрипт `install_dependencies.sh` устанавливает:

- **Java 21 OpenJDK** - для компиляции Java кода
- **Android SDK** - для разработки Android приложений
- **Android NDK r25b** - для компиляции C++ кода
- **Дополнительные инструменты** - CMake, Git, Protobuf и др.

### Сборка Android приложения

Скрипт `build_project.sh` поддерживает следующие опции:

```bash
./build_project.sh [ОПЦИИ]

ОПЦИИ:
  -t, --type TYPE        Тип сборки (debug|release) [по умолчанию: debug]
  -c, --clean            Очистить перед сборкой
  --cpp-only             Собрать только C++ часть
  -v, --verbose          Подробный вывод
  -h, --help             Показать справку
```

### Сборка C++ библиотеки

Скрипт `build_cpp_only.sh` поддерживает:

```bash
./build_cpp_only.sh [ОПЦИИ]

ОПЦИИ:
  -a, --abi ABI          Архитектура (arm64-v8a|armeabi-v7a|x86|x86_64)
  -p, --platform PLAT    Платформа Android (android-21|android-23|etc)
  -t, --type TYPE         Тип сборки (Debug|Release)
  -c, --clean             Очистить перед сборкой
  -v, --verbose           Подробный вывод
  -h, --help              Показать справку
```

## Результаты сборки

### Android приложение

После успешной сборки APK файлы находятся в:
- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-release.apk`

### C++ библиотека

C++ библиотека создается в:
- `app/src/main/cpp/build_standalone/libzmq_reader.so`
- Копируется в `app/src/main/jniLibs/arm64-v8a/libzmq_reader.so`

## Установка на устройство

```bash
# Установка debug версии
adb install app/build/outputs/apk/debug/app-debug.apk

# Установка release версии
adb install app/build/outputs/apk/release/app-release.apk
```

## Устранение неполадок

### Ошибки линковки

Если возникают ошибки линковки с protobuf:

1. Убедитесь, что NDK версии 25.1.8937393
2. Проверьте, что библиотеки protobuf совместимы с версией NDK

### Ошибки компиляции

1. Проверьте версию Java (должна быть 21)
2. Убедитесь, что все зависимости установлены
3. Запустите `./install_dependencies.sh` повторно

### Проблемы с правами

```bash
# Сделать скрипты исполняемыми
chmod +x *.sh

# Если нужно, запустить с sudo
sudo ./install_dependencies.sh
```

## Переменные окружения

Скрипты автоматически настраивают:

```bash
export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
export ANDROID_HOME=/usr/lib/android-sdk
export ANDROID_NDK_ROOT=/opt/android-ndk-r25b
```

## Поддержка архитектур

Поддерживаемые архитектуры:
- `arm64-v8a` (64-bit ARM) - по умолчанию
- `armeabi-v7a` (32-bit ARM)
- `x86` (32-bit x86)
- `x86_64` (64-bit x86)

## Требования к системе

- Ubuntu 20.04+ или аналогичная Linux система
- Минимум 4GB RAM
- 10GB свободного места на диске
- Интернет соединение для загрузки зависимостей
