#!/bin/bash
# Скрипт для беспроводного подключения ADB к устройству

ADB_PORT="5555"

# Функция для получения IP адреса устройства
get_device_ip() {
    # Получаем IP адрес WiFi интерфейса (wlan0 или wlan2)
    local ip=$(adb shell "ip addr | grep 'inet ' | grep -v '127.0.0.1' | grep 'wlan'" | head -1 | awk '{print $2}' | cut -d'/' -f1)

    if [ -z "$ip" ]; then
        # Если wlan не найден, пробуем любой не-loopback интерфейс
        ip=$(adb shell "ip addr | grep 'inet ' | grep -v '127.0.0.1' | grep -v 'rmnet'" | head -1 | awk '{print $2}' | cut -d'/' -f1)
    fi

    echo "$ip"
}

echo "🔌 ADB Wireless Connection Script"
echo "=================================="
echo ""

# Определяем IP адрес (можно переопределить через переменную окружения)
if [ -z "$DEVICE_IP" ]; then
    echo "🔍 Автоматическое определение IP адреса..."
    DEVICE_IP=$(get_device_ip)

    if [ -z "$DEVICE_IP" ]; then
        echo "❌ Не удалось определить IP адрес устройства"
        echo "💡 Убедитесь, что устройство подключено по USB и WiFi включен"
        exit 1
    fi
    echo "✅ Обнаружен IP: $DEVICE_IP"
else
    echo "📌 Использую заданный IP: $DEVICE_IP"
fi

echo ""

# Проверяем, подключено ли устройство
if adb devices | grep -q "$DEVICE_IP:$ADB_PORT"; then
    echo "✅ Устройство уже подключено по Wi-Fi: $DEVICE_IP:$ADB_PORT"
    adb devices
    exit 0
fi

# Если есть USB подключение, переводим в TCP/IP режим
USB_DEVICE=$(adb devices | grep -v "List of devices" | grep -v ":" | grep "device$")
if [ ! -z "$USB_DEVICE" ]; then
    echo "📱 Найдено USB подключение: $USB_DEVICE"
    echo "🔄 Переключаем на TCP/IP режим..."
    adb tcpip $ADB_PORT
    sleep 3
    echo "⏳ Ожидание перезапуска ADB daemon..."
fi

# Подключаемся по Wi-Fi
echo "📡 Подключаемся к $DEVICE_IP:$ADB_PORT..."
adb connect $DEVICE_IP:$ADB_PORT

sleep 1

# Проверяем результат
echo ""
echo "📋 Список устройств:"
adb devices

if adb devices | grep -q "$DEVICE_IP:$ADB_PORT"; then
    echo ""
    echo "✅ Успешно подключено!"
    echo "💡 Теперь можно отключить USB кабель и использовать его для Panda"
else
    echo ""
    echo "❌ Не удалось подключиться"
    echo "Убедитесь, что:"
    echo "  1. Телефон и компьютер в одной Wi-Fi сети"
    echo "  2. IP адрес правильный (текущий: $DEVICE_IP)"
    echo "  3. USB debugging включен"
fi
