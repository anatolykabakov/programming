#!/usr/bin/env python3
"""
Функции для расчета траектории автомобиля различными методами.

Поддерживаемые методы:
1. Одометрия (Steering) - классический bicycle model
2. IMU (Gyroscope) - прямое измерение yaw rate
3. Fusion (IMU + Steering) - sensor fusion с G-H фильтром
"""

import numpy as np
import bisect
from typing import List, Tuple

from vehicle_model import VehicleModel, normalize_angle
from gps_utils import gps_to_local_coords
from ekf import VehicleEKF


def calculate_trajectory_imu(
    wheel: List, imu_yaw_rate: np.ndarray, imu_timestamps: np.ndarray, initial_yaw: float = 0.0
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Вычисляет траекторию автомобиля используя данные IMU (гироскоп).

    Метод:
    - Скорость берется из одометрии (wheel speed)
    - Yaw rate (угловая скорость) берется из гироскопа IMU

    Args:
        wheel: Список данных колес [[timestamp, vr, vl, hr, hl], ...]
        imu_yaw_rate: numpy array (N,) - угловая скорость из IMU (gz_vehicle)
        imu_timestamps: numpy array (N,) - timestamps IMU данных
        initial_yaw: Начальная ориентация в радианах

    Returns:
        Tuple (x_array, y_array) - координаты траектории
    """
    wheel_array = np.array(wheel)
    if len(wheel_array) < 2:
        return np.array([0]), np.array([0])

    x_list, y_list = [0.0], [0.0]
    yaw = float(initial_yaw)

    prev_time = wheel_array[0, 0]

    for i in range(1, len(wheel_array)):
        curr_time = wheel_array[i, 0]
        dt = (curr_time - prev_time) / 1000.0  # В секунды

        if dt < 0.0001 or dt > 1.0:  # Фильтр аномалий
            continue

        # Скорость из одометрии
        # Формат: [timestamp, vr_kmh, vl_kmh, hr_kmh, hl_kmh]
        v_kmh = np.mean(wheel_array[i, 1:5])  # Средняя скорость колес в км/ч
        v = v_kmh / 3.6  # Конвертация км/ч → м/с

        # Yaw rate из IMU (интерполяция на timestamp колес)
        idx = bisect.bisect_left(imu_timestamps, curr_time)
        if idx >= len(imu_yaw_rate):
            idx = len(imu_yaw_rate) - 1
        elif idx > 0 and idx < len(imu_timestamps):
            # Линейная интерполяция между точками
            t1, t2 = imu_timestamps[idx - 1], imu_timestamps[idx]
            if t2 != t1:
                alpha = (curr_time - t1) / (t2 - t1)
                omega = imu_yaw_rate[idx - 1] * (1 - alpha) + imu_yaw_rate[idx] * alpha
            else:
                omega = imu_yaw_rate[idx]
        else:
            omega = imu_yaw_rate[idx]

        # Обновление состояния
        # (Инверсия знака уже применена в process_imu_for_odometry)
        yaw += omega * dt

        dx = v * np.cos(yaw) * dt
        dy = v * np.sin(yaw) * dt

        x_list.append(x_list[-1] + dx)
        y_list.append(y_list[-1] + dy)

        prev_time = curr_time

    return np.array(x_list), np.array(y_list)


def calculate_trajectory_fusion(
    wheel: List,
    steering: List,
    gear: List,
    imu_yaw_rate: np.ndarray,
    imu_timestamps: np.ndarray,
    wheelbase: float = 2.636,
    initial_yaw: float = 0.0,
    alpha: float = 0.7,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Вычисляет траекторию используя sensor fusion (объединение IMU + одометрии).

    Метод: G-H фильтр (простой alpha-beta filter)
    yaw_rate_fusion = alpha * yaw_rate_imu + (1 - alpha) * yaw_rate_odom

    где:
    - yaw_rate_imu: угловая скорость из гироскопа (прямое измерение)
    - yaw_rate_odom: угловая скорость из bicycle model (v * tan(δ) / L)
    - alpha: вес IMU (0.7 = больше доверяем IMU, 0.3 = больше доверяем одометрии)

    Args:
        wheel: Список данных колес [[timestamp, vr, vl, hr, hl], ...]
        steering: Список данных руля [[timestamp, angle_abs, sign], ...]
        gear: Список данных передач [[timestamp, gear_name, gear_value], ...]
        imu_yaw_rate: numpy array (N,) - угловая скорость из IMU (gz_vehicle)
        imu_timestamps: numpy array (N,) - timestamps IMU данных
        wheelbase: Колесная база автомобиля в метрах
        initial_yaw: Начальная ориентация в радианах
        alpha: вес IMU в fusion (0-1), где 1 = только IMU, 0 = только одометрия

    Returns:
        Tuple (x_array, y_array) - координаты траектории

    Пример:
        # alpha = 0.7: 70% IMU, 30% одометрия
        # alpha = 0.5: 50/50 (равный вес)
        # alpha = 0.3: 30% IMU, 70% одометрия
    """
    # Создаем модель автомобиля
    vehicle_model = VehicleModel(wheelbase=wheelbase, initial_yaw=initial_yaw)

    wheel_array = np.array(wheel)
    if len(wheel_array) < 2:
        return np.array([0]), np.array([0])

    # Подготовка данных руля
    if steering is not None and len(steering) > 0:
        steering_array = np.array(steering)
        steering_timestamps = steering_array[:, 0].astype(int).tolist()
        steering_values = steering_array[:, 1:]
    else:
        steering_timestamps = []
        steering_values = np.array([])

    # Подготовка данных передач
    if gear is not None and len(gear) > 0:
        gear_array = np.array(gear, dtype=object)
        gear_timestamps = [int(g[0]) for g in gear]
        gear_names = [g[1] for g in gear]
    else:
        gear_timestamps = []
        gear_names = []

    # Предварительное выделение массивов
    n = len(wheel_array)
    x_arr = np.zeros(n)
    y_arr = np.zeros(n)

    timestamps = wheel_array[:, 0]
    dt_arr = np.diff(timestamps) / 1e3  # секунды
    valid_indices = np.where(dt_arr >= 0.0001)[0] + 1

    result_idx = 0
    yaw = initial_yaw

    print(f"Fusion обработка {len(valid_indices)} точек (alpha={alpha:.1f})...")

    for i in valid_indices:
        t = int(timestamps[i])
        vr, vl, hr, hl = wheel_array[i, 1:5]
        dt = dt_arr[i - 1]

        # Скорость автомобиля
        v_kmh = (vr + vl + hr + hl) / 4.0
        v = v_kmh / 3.6  # км/ч → м/с

        # Получаем угол руля
        steering_angle_abs = 0.0
        steering_angle_sign = 0.0
        if steering_timestamps:
            idx = bisect.bisect_left(steering_timestamps, t)
            if idx >= len(steering_timestamps):
                idx = len(steering_timestamps) - 1
            elif idx > 0 and abs(steering_timestamps[idx - 1] - t) < abs(
                steering_timestamps[idx] - t
            ):
                idx -= 1
            steering_angle_abs = steering_values[idx, 0]
            steering_angle_sign = steering_values[idx, 1]

        # Получаем передачу
        current_gear_name = "DRIVE"
        if gear_timestamps:
            idx = bisect.bisect_left(gear_timestamps, t)
            if idx >= len(gear_timestamps):
                idx = len(gear_timestamps) - 1
            elif idx > 0 and abs(gear_timestamps[idx - 1] - t) < abs(gear_timestamps[idx] - t):
                idx -= 1
            current_gear_name = gear_names[idx]

        # Yaw rate из ОДОМЕТРИИ (bicycle model)
        steering_angle_deg = steering_angle_abs * vehicle_model.unit_to_degrees
        if steering_angle_sign != 0:
            steering_angle_deg = -steering_angle_deg
        steering_angle_rad = np.radians(steering_angle_deg)

        direction = -1.0 if current_gear_name == "REVERSE" else 1.0
        v_directed = v * direction

        if abs(steering_angle_rad) >= 0.001 and abs(v_directed) > 0.01:
            yaw_rate_odom = v_directed * np.tan(steering_angle_rad) / wheelbase
        else:
            yaw_rate_odom = 0.0

        # Yaw rate из IMU (интерполяция)
        idx = bisect.bisect_left(imu_timestamps, t)
        if idx >= len(imu_yaw_rate):
            idx = len(imu_yaw_rate) - 1
        elif idx > 0 and idx < len(imu_timestamps):
            t1, t2 = imu_timestamps[idx - 1], imu_timestamps[idx]
            if t2 != t1:
                alpha_interp = (t - t1) / (t2 - t1)
                yaw_rate_imu = (
                    imu_yaw_rate[idx - 1] * (1 - alpha_interp) + imu_yaw_rate[idx] * alpha_interp
                )
            else:
                yaw_rate_imu = imu_yaw_rate[idx]
        else:
            yaw_rate_imu = imu_yaw_rate[idx]

        # ===== G-H ФИЛЬТР (SENSOR FUSION) =====
        # ======================================
        # Объединяем yaw_rate из двух источников:
        #   alpha = вес IMU (0.7 = больше доверяем IMU)
        #   (1-alpha) = вес одометрии (0.3)
        yaw_rate_fusion = alpha * yaw_rate_imu + (1 - alpha) * yaw_rate_odom

        # Обновление состояния с fusion yaw_rate
        yaw += yaw_rate_fusion * dt
        yaw = normalize_angle(yaw)

        dx = v_directed * np.cos(yaw) * dt
        dy = v_directed * np.sin(yaw) * dt

        x_arr[result_idx] = x_arr[result_idx - 1] + dx if result_idx > 0 else 0
        y_arr[result_idx] = y_arr[result_idx - 1] + dy if result_idx > 0 else 0
        result_idx += 1

    print(f"Fusion траектория: {result_idx} точек (IMU вес={alpha:.1%}, Odom вес={(1-alpha):.1%})")

    return x_arr[:result_idx], y_arr[:result_idx]


def calculate_trajectory(
    wheel: List, steering: List, gear: List, wheelbase: float = 2.636, initial_yaw: float = 0.0
) -> Tuple[List[float], List[float]]:
    """
    Вычисляет траекторию автомобиля по данным одометрии.
    Использует VehicleModel для обновления состояния.

    Args:
        wheel: Список данных колес [[timestamp, vr, vl, hr, hl], ...]
        steering: Список данных руля [[timestamp, angle_abs, sign], ...]
        gear: Список данных передач [[timestamp, gear_name, gear_value], ...]
        wheelbase: Колесная база автомобиля в метрах
        initial_yaw: Начальная ориентация в радианах (0 = восток, π/2 = север)

    Returns:
        Tuple (x_list, y_list) - координаты траектории
    """
    # Создаем модель автомобиля с начальной ориентацией
    vehicle_model = VehicleModel(wheelbase=wheelbase, initial_yaw=initial_yaw)

    # Конвертируем в NumPy массивы для быстрой обработки
    wheel_array = np.array(wheel)
    if len(wheel_array) < 2:
        return [0], [0]

    # Создаем отсортированные массивы для бинарного поиска
    if steering is not None and len(steering) > 0:
        steering_array = np.array(steering)
        steering_timestamps = steering_array[:, 0].astype(int).tolist()
        steering_values = steering_array[:, 1:]
    else:
        steering_timestamps = []
        steering_values = np.array([])

    if gear is not None and len(gear) > 0:
        gear_array = np.array(gear, dtype=object)
        gear_timestamps = [int(g[0]) for g in gear]
        gear_names = [g[1] for g in gear]
    else:
        gear_timestamps = []
        gear_names = []

    # Предварительное выделение массивов (быстрее чем append)
    n = len(wheel_array)
    x_arr = np.zeros(n)
    y_arr = np.zeros(n)

    # Вычисление dt
    timestamps = wheel_array[:, 0]
    dt_arr = np.diff(timestamps) / 1e3  # секунды

    # Фильтр малых dt (0.0001 сек = 0.1 мс)
    # Для высокочастотных данных (~100-1000 Hz) используем малый порог
    valid_indices = np.where(dt_arr >= 0.0001)[0] + 1

    result_idx = 0

    print(f"Обработка {len(valid_indices)} точек траектории...")

    for i in valid_indices:
        t = int(timestamps[i])
        vr, vl, hr, hl = wheel_array[i, 1:5]
        dt = dt_arr[i - 1]

        # Быстрый поиск ближайших значений руля и передачи
        # Используем бинарный поиск (bisect) - O(log n) вместо O(n)
        steering_angle_abs = 0.0
        steering_angle_sign = 0.0
        if steering_timestamps:
            idx = bisect.bisect_left(steering_timestamps, t)
            if idx >= len(steering_timestamps):
                idx = len(steering_timestamps) - 1
            elif idx > 0 and abs(steering_timestamps[idx - 1] - t) < abs(
                steering_timestamps[idx] - t
            ):
                idx -= 1
            steering_angle_abs = steering_values[idx, 0]
            steering_angle_sign = steering_values[idx, 1]

        current_gear_name = "DRIVE"
        if gear_timestamps:
            idx = bisect.bisect_left(gear_timestamps, t)
            if idx >= len(gear_timestamps):
                idx = len(gear_timestamps) - 1
            elif idx > 0 and abs(gear_timestamps[idx - 1] - t) < abs(gear_timestamps[idx] - t):
                idx -= 1
            current_gear_name = gear_names[idx]

        # Обновить состояние автомобиля через VehicleModel
        x, y, yaw = vehicle_model.update(
            vr, vl, hr, hl, steering_angle_abs, steering_angle_sign, current_gear_name, dt
        )

        x_arr[result_idx] = x
        y_arr[result_idx] = y
        result_idx += 1

    # Обрезаем массивы до фактического размера
    x_list = x_arr[:result_idx].tolist()
    y_list = y_arr[:result_idx].tolist()

    print(f"Траектория рассчитана: {result_idx} точек")

    return x_list, y_list


def calculate_trajectory_gps_fusion(
    wheel: List,
    steering: List,
    gear: List,
    imu_yaw_rate: np.ndarray,
    imu_timestamps: np.ndarray,
    gps_data: np.ndarray,
    wheelbase: float = 2.636,
    initial_yaw: float = 0.0,
    alpha_imu: float = 0.7,
    gps_position_gain: float = 0.1,
    gps_yaw_gain: float = 0.05,
    gps_correction_interval: float = 5.0,
    min_gps_speed: float = 1.0,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Вычисляет траекторию с GPS loose coupling (периодическая коррекция по GPS).

    Метод:
    1. Основная траектория: Fusion (alpha*IMU + (1-alpha)*Steering)
    2. Периодическая коррекция по GPS (каждые N секунд):
       - Коррекция позиции (x, y)
       - Коррекция ориентации (yaw)
    3. Мягкая коррекция (малые коэффициенты gain)

    Формулы:
        yaw_rate = alpha_imu * yaw_rate_imu + (1 - alpha_imu) * yaw_rate_odom

        if time_for_gps_correction:
            x += gps_position_gain * (gps_x - x)
            y += gps_position_gain * (gps_y - y)
            yaw += gps_yaw_gain * normalize_angle(gps_heading - yaw)

    Args:
        wheel: Список данных колес [[timestamp, vr, vl, hr, hl], ...]
        steering: Список данных руля [[timestamp, angle_abs, sign], ...]
        gear: Список данных передач [[timestamp, gear_name, gear_value], ...]
        imu_yaw_rate: numpy array (N,) - угловая скорость из IMU
        imu_timestamps: numpy array (N,) - timestamps IMU
        gps_data: numpy array (M, 5) [timestamp, lat, lon, alt, speed]
        wheelbase: Колесная база в метрах
        initial_yaw: Начальная ориентация в радианах
        alpha_imu: вес IMU в fusion (0-1)
        gps_position_gain: коэффициент коррекции позиции GPS (0-1, рекомендую 0.05-0.2)
        gps_yaw_gain: коэффициент коррекции yaw по GPS (0-1, рекомендую 0.02-0.1)
        gps_correction_interval: интервал коррекции GPS в секундах
        min_gps_speed: минимальная скорость для использования GPS heading (м/с)

    Returns:
        Tuple (x_array, y_array) - координаты траектории с GPS коррекцией

    Пример:
        x, y = calculate_trajectory_gps_fusion(
            wheel, steering, gear, imu_yaw_rate, imu_timestamps, gps_data,
            alpha_imu=0.7,           # 70% IMU, 30% steering
            gps_position_gain=0.1,   # 10% коррекция позиции каждые 5с
            gps_yaw_gain=0.05,       # 5% коррекция yaw
            gps_correction_interval=5.0
        )
    """
    # Создаем модель автомобиля
    vehicle_model = VehicleModel(wheelbase=wheelbase, initial_yaw=initial_yaw)

    wheel_array = np.array(wheel)
    if len(wheel_array) < 2:
        return np.array([0]), np.array([0])

    # Конвертируем GPS в локальные координаты
    x_gps, y_gps = gps_to_local_coords(gps_data, origin_idx=0)
    gps_timestamps = gps_data[:, 0]
    gps_speeds = gps_data[:, 4]  # Скорость GPS

    # Подготовка данных руля и передач
    if steering is not None and len(steering) > 0:
        steering_array = np.array(steering)
        steering_timestamps = steering_array[:, 0].astype(int).tolist()
        steering_values = steering_array[:, 1:]
    else:
        steering_timestamps = []
        steering_values = np.array([])

    if gear is not None and len(gear) > 0:
        gear_array = np.array(gear, dtype=object)
        gear_timestamps = [int(g[0]) for g in gear]
        gear_names = [g[1] for g in gear]
    else:
        gear_timestamps = []
        gear_names = []

    # Предварительное выделение массивов
    n = len(wheel_array)
    x_arr = np.zeros(n)
    y_arr = np.zeros(n)

    timestamps = wheel_array[:, 0]
    dt_arr = np.diff(timestamps) / 1e3  # секунды
    valid_indices = np.where(dt_arr >= 0.0001)[0] + 1

    result_idx = 0
    yaw = initial_yaw
    last_gps_correction_time = timestamps[0]
    gps_corrections_count = 0
    gps_corrections_skipped_position = 0  # Пропущено из-за большой ошибки позиции
    gps_corrections_skipped_yaw_rate = 0  # Пропущено из-за резкого маневра
    gps_corrections_reduced_gain = 0  # Коррекция с уменьшенным gain

    print(f"GPS Fusion обработка {len(valid_indices)} точек...")
    print(
        f"  Параметры: alpha_IMU={alpha_imu:.1f}, GPS_pos_gain={gps_position_gain:.2f}, GPS_yaw_gain={gps_yaw_gain:.3f}"
    )
    print(f"  GPS коррекция каждые {gps_correction_interval:.1f}с")
    print(f"  Защита: адаптивный gain (0-20м→100%, 20-50м→50%, 50-200м→20%, >200м→skip)")
    print(f"          yaw только при плавном движении (<17°/с), yaw_error<45°")

    for i in valid_indices:
        t = int(timestamps[i])
        vr, vl, hr, hl = wheel_array[i, 1:5]
        dt = dt_arr[i - 1]

        # Скорость автомобиля
        v_kmh = (vr + vl + hr + hl) / 4.0
        v = v_kmh / 3.6  # км/ч → м/с

        # Получаем угол руля и передачу (как в fusion)
        steering_angle_abs = 0.0
        steering_angle_sign = 0.0
        if steering_timestamps:
            idx = bisect.bisect_left(steering_timestamps, t)
            if idx >= len(steering_timestamps):
                idx = len(steering_timestamps) - 1
            elif idx > 0 and abs(steering_timestamps[idx - 1] - t) < abs(
                steering_timestamps[idx] - t
            ):
                idx -= 1
            steering_angle_abs = steering_values[idx, 0]
            steering_angle_sign = steering_values[idx, 1]

        current_gear_name = "DRIVE"
        if gear_timestamps:
            idx = bisect.bisect_left(gear_timestamps, t)
            if idx >= len(gear_timestamps):
                idx = len(gear_timestamps) - 1
            elif idx > 0 and abs(gear_timestamps[idx - 1] - t) < abs(gear_timestamps[idx] - t):
                idx -= 1
            current_gear_name = gear_names[idx]

        # Yaw rate из одометрии (bicycle model)
        steering_angle_deg = steering_angle_abs * vehicle_model.unit_to_degrees
        if steering_angle_sign != 0:
            steering_angle_deg = -steering_angle_deg
        steering_angle_rad = np.radians(steering_angle_deg)

        direction = -1.0 if current_gear_name == "REVERSE" else 1.0
        v_directed = v * direction

        if abs(steering_angle_rad) >= 0.001 and abs(v_directed) > 0.01:
            yaw_rate_odom = v_directed * np.tan(steering_angle_rad) / wheelbase
        else:
            yaw_rate_odom = 0.0

        # Yaw rate из IMU (интерполяция)
        idx = bisect.bisect_left(imu_timestamps, t)
        if idx >= len(imu_yaw_rate):
            idx = len(imu_yaw_rate) - 1
        elif idx > 0 and idx < len(imu_timestamps):
            t1, t2 = imu_timestamps[idx - 1], imu_timestamps[idx]
            if t2 != t1:
                alpha_interp = (t - t1) / (t2 - t1)
                yaw_rate_imu = (
                    imu_yaw_rate[idx - 1] * (1 - alpha_interp) + imu_yaw_rate[idx] * alpha_interp
                )
            else:
                yaw_rate_imu = imu_yaw_rate[idx]
        else:
            yaw_rate_imu = imu_yaw_rate[idx]

        # Sensor fusion (IMU + Steering)
        yaw_rate_fusion = alpha_imu * yaw_rate_imu + (1 - alpha_imu) * yaw_rate_odom

        # Обновление состояния
        yaw += yaw_rate_fusion * dt
        yaw = normalize_angle(yaw)

        dx = v_directed * np.cos(yaw) * dt
        dy = v_directed * np.sin(yaw) * dt

        x_current = (x_arr[result_idx - 1] if result_idx > 0 else 0) + dx
        y_current = (y_arr[result_idx - 1] if result_idx > 0 else 0) + dy

        # ===== GPS КОРРЕКЦИЯ (периодическая с проверками) =====
        # =======================================================
        time_since_last_correction = (t - last_gps_correction_time) / 1000.0

        if time_since_last_correction >= gps_correction_interval:
            # Находим ближайшую GPS точку
            gps_idx = bisect.bisect_left(gps_timestamps, t)
            if gps_idx >= len(gps_timestamps):
                gps_idx = len(gps_timestamps) - 1
            elif gps_idx > 0:
                # Выбираем ближайшую по времени
                if abs(gps_timestamps[gps_idx - 1] - t) < abs(gps_timestamps[gps_idx] - t):
                    gps_idx = gps_idx - 1

            # Проверяем скорость GPS (для надежного heading нужно движение)
            if gps_idx < len(gps_speeds) and gps_speeds[gps_idx] >= min_gps_speed:
                # Коррекция позиции
                gps_x_current = x_gps[gps_idx]
                gps_y_current = y_gps[gps_idx]

                dx_error = gps_x_current - x_current
                dy_error = gps_y_current - y_current
                position_error = np.sqrt(dx_error ** 2 + dy_error ** 2)

                # ===== ПРОВЕРКА 1: Адаптивный gain в зависимости от ошибки =====
                # ================================================================
                # Стратегия: НЕ отклоняем коррекцию, а адаптируем gain
                #
                # Ошибка 0-20м:   gain = 100% (полная коррекция)
                # Ошибка 20-50м:  gain = 50%  (половина)
                # Ошибка 50-100м: gain = 20%  (малая коррекция)
                # Ошибка >200м:   gain = 0%   (пропуск - аномалия GPS)

                max_acceptable_error = 200.0  # метров (порог аномалии GPS)

                if position_error > max_acceptable_error:
                    # Аномально большая ошибка (>200м) - GPS явно сбоит
                    gps_corrections_skipped_position += 1
                else:
                    # Адаптивный gain: плавное снижение с ростом ошибки
                    if position_error < 20.0:
                        adaptive_position_gain = gps_position_gain  # 100%
                    elif position_error < 50.0:
                        adaptive_position_gain = gps_position_gain * 0.5  # 50%
                        gps_corrections_reduced_gain += 1
                    else:  # 50-200м
                        adaptive_position_gain = gps_position_gain * 0.2  # 20%
                        gps_corrections_reduced_gain += 1

                    # Мягкая коррекция позиции
                    x_current += adaptive_position_gain * dx_error
                    y_current += adaptive_position_gain * dy_error

                    # ===== ПРОВЕРКА 2: Коррекция yaw только при плавном движении =====
                    # ==================================================================
                    # Вычисляем текущую угловую скорость (модуль)
                    current_yaw_rate = abs(yaw_rate_fusion)
                    max_yaw_rate_for_gps_correction = 0.3  # рад/с ≈ 17°/с

                    # Если маневр слишком резкий (парковка, разворот):
                    #   - yaw_rate > 0.3 рад/с (17°/с)
                    #   → Не корректируем yaw (доверяем IMU + Steering)
                    if current_yaw_rate < max_yaw_rate_for_gps_correction:
                        # Коррекция yaw (по направлению движения GPS)
                        # Берем несколько GPS точек для расчета heading
                        if gps_idx >= 2 and gps_idx < len(x_gps) - 2:
                            # Используем 5 точек для уменьшения шума GPS
                            gps_dx = x_gps[gps_idx + 1] - x_gps[gps_idx - 1]
                            gps_dy = y_gps[gps_idx + 1] - y_gps[gps_idx - 1]

                            # Минимальное смещение для надежного heading
                            if gps_dx ** 2 + gps_dy ** 2 > 4.0:  # > 2 метров
                                gps_heading = np.arctan2(gps_dy, gps_dx)
                                yaw_error = normalize_angle(gps_heading - yaw)

                                # ===== ПРОВЕРКА 3: Ошибка yaw не слишком большая =====
                                # ======================================================
                                # Если ошибка > 45°, возможно:
                                #   - Резкий поворот на парковке
                                #   - GPS шум
                                # → Снижаем gain или пропускаем
                                max_acceptable_yaw_error = np.radians(45)  # 45°

                                if abs(yaw_error) > max_acceptable_yaw_error:
                                    # Слишком большая ошибка yaw - используем малый gain
                                    adaptive_yaw_gain = gps_yaw_gain * 0.2
                                    gps_corrections_reduced_gain += 1
                                else:
                                    adaptive_yaw_gain = gps_yaw_gain

                                # Мягкая коррекция yaw
                                yaw += adaptive_yaw_gain * yaw_error
                                yaw = normalize_angle(yaw)
                    else:
                        # Резкий маневр - пропускаем коррекцию yaw
                        gps_corrections_skipped_yaw_rate += 1

                    last_gps_correction_time = t
                    gps_corrections_count += 1

        x_arr[result_idx] = x_current
        y_arr[result_idx] = y_current
        result_idx += 1

    print(f"GPS Fusion траектория: {result_idx} точек")
    print(f"  ✅ GPS коррекций применено: {gps_corrections_count}")
    print(f"  🔽 Коррекций со сниженным gain: {gps_corrections_reduced_gain}")
    print(f"  ⚠️  Пропущено (аномалия >200м): {gps_corrections_skipped_position}")
    print(f"  ⚠️  Пропущено yaw (резкий маневр >17°/с): {gps_corrections_skipped_yaw_rate}")
    print(
        f"  IMU вес={alpha_imu:.1%}, Steering вес={(1-alpha_imu):.1%}, GPS коррекция={gps_position_gain:.1%}"
    )

    return x_arr[:result_idx], y_arr[:result_idx]


def calculate_trajectory_ekf(
    wheel: List,
    steering: List,
    gear: List,
    imu_yaw_rate: np.ndarray,
    imu_timestamps: np.ndarray,
    gps_data: np.ndarray,
    wheelbase: float = 2.636,
    initial_yaw: float = 0.0,
    alpha_imu: float = 0.7,
    gps_update_interval: float = 1.0,
    imu_update_interval: float = 0.01,
) -> Tuple[np.ndarray, np.ndarray, VehicleEKF]:
    """
    Вычисляет траекторию с использованием Extended Kalman Filter (EKF).

    Метод (правильная архитектура - без "information incest"):
    1. Prediction (100Hz): Bicycle model с ТОЛЬКО одометрией (steering angle)
    2. Update GPS (1Hz): Коррекция позиции (x, y) с адаптивным Kalman gain
    3. Update IMU (100Hz): Коррекция yaw_rate по IMU

    ВАЖНО: IMU используется ТОЛЬКО в update, не в prediction!
    Это предотвращает двойное использование данных IMU.

    Преимущества:
      ✅ Быстрая конвергенция в начале (большой Kalman gain)
      ✅ Малая коррекция при стабильном состоянии (малый gain)
      ✅ Автоматическое отклонение GPS аномалий
      ✅ Правильное разделение источников данных
      ✅ Оптимальное объединение всех датчиков

    Args:
        wheel: Список данных колес [[timestamp, vr, vl, hr, hl], ...]
        steering: Список данных руля [[timestamp, angle_abs, sign], ...]
        gear: Список данных передач [[timestamp, gear_name, gear_value], ...]
        imu_yaw_rate: numpy array (N,) - угловая скорость из IMU
        imu_timestamps: numpy array (N,) - timestamps IMU
        gps_data: numpy array (M, 5) [timestamp, lat, lon, alt, speed]
        wheelbase: Колесная база в метрах
        initial_yaw: Начальная ориентация в радианах
        alpha_imu: (не используется, оставлен для совместимости API)
        gps_update_interval: интервал GPS update (секунды)
        imu_update_interval: интервал IMU update (секунды)

    Returns:
        Tuple (x_array, y_array, ekf) - координаты траектории и EKF объект

    Пример:
        x, y, ekf = calculate_trajectory_ekf(
            wheel, steering, gear, imu_yaw_rate, imu_timestamps, gps_data,
            gps_update_interval=1.0,  # GPS каждую секунду
            imu_update_interval=0.01   # IMU каждые 10мс
        )
    """

    # Инициализация EKF
    ekf = VehicleEKF(
        initial_x=0.0,
        initial_y=0.0,
        initial_yaw=initial_yaw,
        initial_v=0.0,
        initial_yaw_rate=0.0,
        wheelbase=wheelbase,
        # Шум процесса (настройте под свои данные)
        process_noise_pos=0.1,  # м
        process_noise_yaw=0.01,  # рад
        process_noise_v=0.5,  # м/с
        process_noise_yaw_rate=0.05,  # рад/с
        # Шум измерений
        gps_noise_pos=5.0,  # м (GPS точность)
        imu_noise_yaw_rate=0.02,  # рад/с (IMU точность)
        # Начальная неопределенность (большая → быстро доверяем измерениям)
        initial_pos_uncertainty=10.0,  # м
        initial_yaw_uncertainty=0.5,  # рад (~30°)
        initial_v_uncertainty=2.0,  # м/с
        initial_yaw_rate_uncertainty=0.1,  # рад/с
    )

    wheel_array = np.array(wheel)
    if len(wheel_array) < 2:
        return np.array([0]), np.array([0]), ekf

    # Конвертируем GPS в локальные координаты
    x_gps, y_gps = gps_to_local_coords(gps_data, origin_idx=0)
    gps_timestamps = gps_data[:, 0]

    # Подготовка данных руля и передач
    if steering is not None and len(steering) > 0:
        steering_array = np.array(steering)
        steering_timestamps = steering_array[:, 0].astype(int).tolist()
        steering_values = steering_array[:, 1:]
    else:
        steering_timestamps = []
        steering_values = np.array([])

    if gear is not None and len(gear) > 0:
        gear_array = np.array(gear, dtype=object)
        gear_timestamps = [int(g[0]) for g in gear]
        gear_names = [g[1] for g in gear]
    else:
        gear_timestamps = []
        gear_names = []

    # Предварительное выделение массивов
    n = len(wheel_array)
    x_arr = np.zeros(n)
    y_arr = np.zeros(n)

    timestamps = wheel_array[:, 0]
    dt_arr = np.diff(timestamps) / 1e3  # секунды
    valid_indices = np.where(dt_arr >= 0.0001)[0] + 1

    result_idx = 0
    last_gps_update_time = timestamps[0]
    last_imu_update_time = timestamps[0]

    print(f"\n{'='*60}")
    print("🎯 EKF ОБРАБОТКА (правильная архитектура)")
    print(f"{'='*60}")
    print(f"Обработка {len(valid_indices)} точек траектории...")
    print(f"  Архитектура:")
    print(f"    Prediction:       ТОЛЬКО одометрия (steering)")
    print(f"    Update GPS:       коррекция позиции (x, y)")
    print(f"    Update IMU:       коррекция yaw_rate")
    print(f"  Параметры:")
    print(f"    Wheelbase:        {wheelbase:.3f} м")
    print(f"    GPS update:       каждые {gps_update_interval:.1f}с")
    print(f"    IMU update:       каждые {imu_update_interval*1000:.0f}мс")
    print(f"  Данные:")
    print(f"    GPS точек:        {len(gps_timestamps)}")
    print(f"    IMU точек:        {len(imu_timestamps)}")
    print(f"{'='*60}\n")

    for i in valid_indices:
        t = int(timestamps[i])
        vr, vl, hr, hl = wheel_array[i, 1:5]
        dt = dt_arr[i - 1]

        # Скорость автомобиля
        v_kmh = (vr + vl + hr + hl) / 4.0
        v = v_kmh / 3.6  # км/ч → м/с

        # Получаем угол руля и передачу
        steering_angle_abs = 0.0
        steering_angle_sign = 0.0
        if steering_timestamps:
            idx = bisect.bisect_left(steering_timestamps, t)
            if idx >= len(steering_timestamps):
                idx = len(steering_timestamps) - 1
            elif idx > 0 and abs(steering_timestamps[idx - 1] - t) < abs(
                steering_timestamps[idx] - t
            ):
                idx -= 1
            steering_angle_abs = steering_values[idx, 0]
            steering_angle_sign = steering_values[idx, 1]

        current_gear_name = "DRIVE"
        if gear_timestamps:
            idx = bisect.bisect_left(gear_timestamps, t)
            if idx >= len(gear_timestamps):
                idx = len(gear_timestamps) - 1
            elif idx > 0 and abs(gear_timestamps[idx - 1] - t) < abs(gear_timestamps[idx] - t):
                idx -= 1
            current_gear_name = gear_names[idx]

        # Вычисляем угол руля
        steering_angle_deg = steering_angle_abs * 0.0122  # unit_to_degrees
        if steering_angle_sign != 0:
            steering_angle_deg = -steering_angle_deg
        steering_angle_rad = np.radians(steering_angle_deg)

        # Направление движения
        direction = -1.0 if current_gear_name == "REVERSE" else 1.0
        v_directed = v * direction

        # Получаем IMU yaw_rate (интерполяция)
        idx = bisect.bisect_left(imu_timestamps, t)
        if idx >= len(imu_yaw_rate):
            idx = len(imu_yaw_rate) - 1
        elif idx > 0 and idx < len(imu_timestamps):
            t1, t2 = imu_timestamps[idx - 1], imu_timestamps[idx]
            if t2 != t1:
                alpha_interp = (t - t1) / (t2 - t1)
                yaw_rate_imu = (
                    imu_yaw_rate[idx - 1] * (1 - alpha_interp) + imu_yaw_rate[idx] * alpha_interp
                )
            else:
                yaw_rate_imu = imu_yaw_rate[idx]
        else:
            yaw_rate_imu = imu_yaw_rate[idx]

        # ===== PREDICTION STEP (только одометрия) =====
        ekf.predict(v_measured=v_directed, steering_angle=steering_angle_rad, dt=dt)

        # ===== GPS UPDATE (периодически) =====
        time_since_gps = (t - last_gps_update_time) / 1000.0
        if time_since_gps >= gps_update_interval:
            # Находим ближайшую GPS точку
            gps_idx = bisect.bisect_left(gps_timestamps, t)
            if gps_idx >= len(gps_timestamps):
                gps_idx = len(gps_timestamps) - 1
            elif gps_idx > 0:
                if abs(gps_timestamps[gps_idx - 1] - t) < abs(gps_timestamps[gps_idx] - t):
                    gps_idx = gps_idx - 1

            # Применяем GPS update
            gps_applied = ekf.update_gps(x_gps[gps_idx], y_gps[gps_idx], max_innovation=50.0)

            if gps_applied:
                last_gps_update_time = t

        # ===== IMU UPDATE (периодически) =====
        time_since_imu = (t - last_imu_update_time) / 1000.0
        if time_since_imu >= imu_update_interval:
            ekf.update_imu(yaw_rate_imu)
            last_imu_update_time = t

        # Сохраняем позицию
        x_current, y_current = ekf.get_position()
        x_arr[result_idx] = x_current
        y_arr[result_idx] = y_current
        result_idx += 1

    # Статистика EKF
    ekf.print_statistics()

    print(f"\nEKF траектория: {result_idx} точек")

    return x_arr[:result_idx], y_arr[:result_idx], ekf
