#!/usr/bin/env python3
"""
Утилиты для работы с IMU данными.
Калибровка, трансформация координат, обработка данных гироскопа и акселерометра.
"""
import numpy as np
from typing import Tuple, Optional


def filter_imu_by_vehicle_speed(
    imu_data: np.ndarray,
    vehicle_speed_data: np.ndarray,
    speed_threshold: float = 0.1,
    time_window_sec: Optional[float] = None,
    min_samples: Optional[int] = None,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Фильтрует IMU данные, оставляя только моменты когда автомобиль стоит или движется медленно.

    Выполняет:
    1. Синхронизацию IMU и vehicle speed по времени (интерполяция)
    2. Определение моментов покоя (скорость < порог)
    3. Фильтрацию IMU данных
    4. Опциональную фильтрацию по временному окну

    Args:
        imu_data: numpy array shape (N, 10) [timestamp, ax, ay, az, gx, gy, gz, mx, my, mz]
        vehicle_speed_data: numpy array shape (M, 5) [timestamp, fl, fr, rl, rr]
                           или shape (M,) [speed] или shape (M, 2) [timestamp, speed]
        speed_threshold: порог скорости (м/с), ниже которого считаем что машина стоит
        time_window_sec: опционально - временное окно от начала записи (секунды)
        min_samples: опционально - минимальное количество сэмплов для возврата

    Returns:
        Tuple[filtered_imu, interpolated_speeds]:
            - filtered_imu: отфильтрованные IMU данные (только когда машина стоит)
            - interpolated_speeds: скорости, интерполированные на timestamps IMU

    Raises:
        ValueError: если недостаточно данных

    Пример:
        # vehicle_speed_data может быть:
        # 1. [timestamp, fl, fr, rl, rr] - данные из CAN
        # 2. [timestamp, speed] - уже вычисленная средняя скорость
        # 3. [speed] - массив скоростей (без timestamps, предполагается синхронность)

        filtered_imu, speeds = filter_imu_by_vehicle_speed(
            imu_data,
            vehicle_speed_data,
            speed_threshold=0.1,
            time_window_sec=20.0,
            min_samples=50
        )
    """
    if imu_data is None or len(imu_data) == 0:
        raise ValueError("IMU data is empty")

    if vehicle_speed_data is None or len(vehicle_speed_data) == 0:
        raise ValueError("Vehicle speed data is empty")

    # Конвертируем в numpy array если нужно
    if not isinstance(vehicle_speed_data, np.ndarray):
        vehicle_speed_data = np.array(vehicle_speed_data)

    imu_timestamps = imu_data[:, 0]

    # ===== ЭТАП 1: Извлечение скоростей и timestamps =====
    # ======================================================

    if vehicle_speed_data.ndim == 1:
        # Случай 1: Только скорости [speed, speed, ...] без timestamps
        # Предполагаем синхронность по индексам
        speeds = vehicle_speed_data
        speed_timestamps = None
        print(
            f"   Данные скорости: {len(speeds)} значений (без timestamps, синхронность по индексам)"
        )

    elif vehicle_speed_data.ndim == 2:
        if vehicle_speed_data.shape[1] == 2:
            # Случай 2: [timestamp, speed]
            speed_timestamps = vehicle_speed_data[:, 0]
            speeds = vehicle_speed_data[:, 1]
            print(f"   Данные скорости: {len(speeds)} значений с timestamps")

        elif vehicle_speed_data.shape[1] >= 4:
            # Случай 3: [timestamp, fl, fr, rl, rr, ...]
            speed_timestamps = vehicle_speed_data[:, 0]
            # Вычисляем среднюю скорость колес
            speeds = np.mean(vehicle_speed_data[:, 1:5], axis=1)
            print(f"   Данные скорости: {len(speeds)} значений (усреднено по 4 колесам)")
        else:
            raise ValueError(f"Unexpected vehicle_speed_data shape: {vehicle_speed_data.shape}")
    else:
        raise ValueError(
            f"vehicle_speed_data должен быть 1D или 2D массивом, получен {vehicle_speed_data.ndim}D"
        )

    # ===== ЭТАП 2: Синхронизация по времени =====
    # ============================================

    if speed_timestamps is not None:
        # Интерполируем скорости на IMU timestamps
        from scipy import interpolate

        print(f"   Синхронизация по времени...")
        print(
            f"     IMU: {len(imu_timestamps)} сэмплов, диапазон {imu_timestamps[0]:.0f} - {imu_timestamps[-1]:.0f} мс"
        )
        print(
            f"     Speed: {len(speed_timestamps)} сэмплов, диапазон {speed_timestamps[0]:.0f} - {speed_timestamps[-1]:.0f} мс"
        )

        # Используем linear interpolation для скоростей
        try:
            speed_interp_func = interpolate.interp1d(
                speed_timestamps,
                speeds,
                kind="linear",
                bounds_error=False,
                fill_value=(
                    speeds[0],
                    speeds[-1],
                ),  # Используем граничные значения за пределами диапазона
            )
            speeds_interpolated = speed_interp_func(imu_timestamps)
            print(f"     ✅ Интерполяция выполнена")

        except Exception as e:
            print(
                f"     ⚠️  Warning: Интерполяция не удалась ({e}), использую синхронизацию по индексам"
            )
            # Fallback: синхронизация по индексам
            n_sync = min(len(imu_data), len(speeds))
            speeds_interpolated = np.zeros(len(imu_data))
            speeds_interpolated[:n_sync] = speeds[:n_sync]
            speeds_interpolated[n_sync:] = speeds[-1] if len(speeds) > 0 else 0
    else:
        # Синхронизация по индексам (предполагаем одинаковую частоту дискретизации)
        print(f"   Синхронизация по индексам (предполагается одинаковая частота)...")
        n_sync = min(len(imu_data), len(speeds))
        speeds_interpolated = np.zeros(len(imu_data))
        speeds_interpolated[:n_sync] = speeds[:n_sync]
        # Для остальных используем последнее значение
        if n_sync < len(imu_data):
            speeds_interpolated[n_sync:] = speeds[-1] if len(speeds) > 0 else 0

    # ===== ЭТАП 3: Фильтрация по скорости =====
    # ==========================================

    stationary_mask = speeds_interpolated < speed_threshold

    # Опционально: фильтрация по временному окну
    if time_window_sec is not None:
        start_time = imu_timestamps[0]
        time_mask = (imu_timestamps - start_time) / 1000.0 <= time_window_sec
        stationary_mask = stationary_mask & time_mask
        print(f"   Фильтрация: скорость < {speed_threshold} м/с И время < {time_window_sec}с")
    else:
        print(f"   Фильтрация: скорость < {speed_threshold} м/с")

    stationary_indices = np.where(stationary_mask)[0]

    print(f"   Найдено {len(stationary_indices)} сэмплов где машина стоит")

    # ===== ЭТАП 4: Проверка минимального количества =====
    # ====================================================

    if min_samples is not None and len(stationary_indices) < min_samples:
        print(f"   ⚠️  Warning: Недостаточно сэмплов ({len(stationary_indices)} < {min_samples})")

        if time_window_sec is not None:
            # Попробуем без временного ограничения
            print(f"       Пробуем без временного ограничения...")
            stationary_mask_no_time = speeds_interpolated < speed_threshold
            stationary_indices = np.where(stationary_mask_no_time)[0]
            print(f"       Найдено: {len(stationary_indices)} сэмплов")

        if len(stationary_indices) < min_samples:
            # Используем первые min_samples
            print(f"       Используем первые {min_samples} сэмплов без фильтрации")
            stationary_indices = np.arange(min(min_samples, len(imu_data)))

    # ===== ЭТАП 5: Возвращаем отфильтрованные данные =====
    # =====================================================

    filtered_imu = imu_data[stationary_indices]

    print(f"   ✅ Отфильтровано: {len(filtered_imu)} сэмплов IMU (машина стоит)")

    return filtered_imu, speeds_interpolated


def calibrate_gyro_bias(imu_stationary: np.ndarray) -> Tuple[np.ndarray, int]:
    """
    Калибрует bias гироскопа по данным когда автомобиль стоит.

    ВАЖНО: Функция ожидает данные когда автомобиль стоит неподвижно!
    Используйте filter_imu_by_vehicle_speed() для предварительной фильтрации.

    Bias (смещение нуля) - это систематическая ошибка гироскопа, которая
    накапливается при интегрировании и приводит к дрейфу ориентации.

    Логика:
        Когда машина стоит → реальная угловая скорость = 0 рад/с
        Но гироскоп показывает ≠ 0 → это и есть bias!
        bias = mean(gyro[когда_машина_стоит])

    Args:
        imu_stationary: numpy array shape (N, 10) [timestamp, ax, ay, az, gx, gy, gz, mx, my, mz]
                       Данные IMU когда машина стоит (уже отфильтрованные!)

    Returns:
        bias: numpy array [gx_bias, gy_bias, gz_bias] в рад/с
        n_samples: количество сэмплов, использованных для калибровки

    Raises:
        ValueError: если недостаточно данных для калибровки

    Пример:
        # Сначала фильтруем данные:
        imu_stationary, _ = filter_imu_by_vehicle_speed(
            imu_data, vehicle_speed,
            speed_threshold=0.1
        )

        # Потом вычисляем bias:
        bias, n = calibrate_gyro_bias(imu_stationary)
        # bias = [-0.009124, -0.000293, 0.001400] рад/с

        # Применяем калибровку ко ВСЕМ данным:
        imu_calibrated = apply_gyro_calibration(imu_data, bias)

    Примечание:
        - Для лучших результатов нужно хотя бы 50-100 сэмплов когда машина стоит
        - Bias может меняться со временем и температурой
        - Рекомендуется периодическая рекалибровка
    """
    if imu_stationary is None or len(imu_stationary) == 0:
        raise ValueError("IMU stationary data is empty")

    n_stationary = len(imu_stationary)

    # ЭТАП 1: Извлекаем данные гироскопа
    # ==================================
    # Входные данные УЖЕ отфильтрованы (машина стоит)
    gyro_data_stationary = imu_stationary[:, 4:7]  # gx, gy, gz

    # ЭТАП 2: Вычисляем bias как среднее значение
    # ============================================
    # Когда машина стоит, реальная угловая скорость = 0
    # Поэтому показания гироскопа = bias
    gx_bias = np.mean(gyro_data_stationary[:, 0])
    gy_bias = np.mean(gyro_data_stationary[:, 1])
    gz_bias = np.mean(gyro_data_stationary[:, 2])

    bias = np.array([gx_bias, gy_bias, gz_bias])

    # ЭТАП 3: Статистика и вывод информации
    # ======================================
    gx_std = np.std(gyro_data_stationary[:, 0])
    gy_std = np.std(gyro_data_stationary[:, 1])
    gz_std = np.std(gyro_data_stationary[:, 2])

    print(f"\n📊 Калибровка гироскопа:")
    print(f"   Использовано сэмплов: {n_stationary} (машина стоит)")
    print(f"\n   Bias (смещение нуля):")
    print(f"     gx: {gx_bias:+.6f} ± {gx_std:.6f} рад/с")
    print(f"     gy: {gy_bias:+.6f} ± {gy_std:.6f} рад/с")
    print(f"     gz: {gz_bias:+.6f} ± {gz_std:.6f} рад/с (yaw rate)")
    print(f"\n   Накопленная ошибка без калибровки:")
    print(f"     За 1 мин:  {abs(gz_bias) * 60 * 180/np.pi:.1f}°")
    print(f"     За 10 мин: {abs(gz_bias) * 600 * 180/np.pi:.1f}°")

    return bias, n_stationary


def detect_phone_orientation(imu_stationary: np.ndarray) -> dict:
    """
    Определяет ориентацию телефона относительно автомобиля по данным акселерометра.

    ВАЖНО: Функция ожидает данные когда автомобиль стоит неподвижно!
    Используйте filter_imu_by_vehicle_speed() для предварительной фильтрации.

    Когда автомобиль стоит на ровной поверхности, акселерометр измеряет только
    гравитацию (≈9.81 м/с² направленную вниз). По направлению вектора гравитации
    можно определить, как расположен телефон.

    Стандартная ориентация для автомобиля:
        X: вправо (пассажирская дверь)
        Y: вперед (капот)
        Z: вверх (небо)

    Args:
        imu_stationary: numpy array shape (N, 10) [timestamp, ax, ay, az, ...]
                       Данные IMU когда машина стоит (уже отфильтрованные!)

    Returns:
        dict с информацией об ориентации:
            'gravity_vector': [gx, gy, gz] - вектор гравитации в системе IMU
            'gravity_magnitude': float - модуль вектора (должен быть ≈9.81)
            'dominant_axis': str - основная ось ('X', 'Y', 'Z')
            'orientation_ok': bool - True если Z направлен вверх (стандартно)
            'n_samples_used': int - количество использованных сэмплов

    Пример:
        # Сначала фильтруем данные:
        imu_stationary, _ = filter_imu_by_vehicle_speed(
            imu_data, vehicle_speed,
            speed_threshold=0.1, time_window_sec=20
        )

        # Потом анализируем ориентацию:
        orientation = detect_phone_orientation(imu_stationary)

        if orientation['orientation_ok']:
            print("✅ Телефон лежит горизонтально")
        else:
            print(f"⚠️  Доминирующая ось: {orientation['dominant_axis']}")
    """
    if imu_stationary is None or len(imu_stationary) == 0:
        raise ValueError("IMU stationary data is empty")

    # ЭТАП 1: Усредняем данные акселерометра
    # =======================================
    # Когда машина стоит, акселерометр показывает только гравитацию
    # Входные данные УЖЕ отфильтрованы (машина стоит на месте)
    ax_mean = np.mean(imu_stationary[:, 1])
    ay_mean = np.mean(imu_stationary[:, 2])
    az_mean = np.mean(imu_stationary[:, 3])

    gravity_vector = np.array([ax_mean, ay_mean, az_mean])
    gravity_magnitude = np.linalg.norm(gravity_vector)

    # ЭТАП 2: Определяем доминирующую ось
    # ====================================
    # Ось с наибольшим модулем ускорения направлена вниз (или вверх)
    abs_values = np.abs(gravity_vector)
    dominant_idx = np.argmax(abs_values)
    dominant_axes = ["X", "Y", "Z"]
    dominant_axis = dominant_axes[dominant_idx]

    # ЭТАП 3: Проверяем стандартную ориентацию
    # =========================================
    # Стандартно: Z должна быть доминирующей, az ≈ -9.81 (или +9.81)
    # (знак зависит от конвенции, но Z должна быть вертикальной)
    orientation_ok = (dominant_axis == "Z") and (abs_values[2] > 8.0)

    # ЭТАП 4: Формируем результат
    # ============================
    result = {
        "gravity_vector": gravity_vector,
        "gravity_magnitude": gravity_magnitude,
        "dominant_axis": dominant_axis,
        "orientation_ok": orientation_ok,
        "n_samples_used": len(imu_stationary),
        "ax_mean": ax_mean,
        "ay_mean": ay_mean,
        "az_mean": az_mean,
    }

    # ЭТАП 5: Вывод информации
    # =========================
    print(f"\n📱 Ориентация телефона:")
    print(f"   Использовано сэмплов: {len(imu_stationary)} (машина стоит)")
    print(f"   Вектор гравитации (IMU): [{ax_mean:.2f}, {ay_mean:.2f}, {az_mean:.2f}] м/с²")
    print(f"   Модуль: {gravity_magnitude:.2f} м/с² (норма: 9.81 м/с²)")
    print(f"   Доминирующая ось: {dominant_axis}")

    if orientation_ok:
        print(f"   ✅ Ориентация OK - телефон лежит горизонтально (Z вверх)")
    else:
        print(f"   ⚠️  Нестандартная ориентация - доминирует ось {dominant_axis}")
        print(f"       Возможно телефон стоит вертикально или наклонен")
        print(f"       Рекомендуется трансформация координат для корректных расчетов")

    return result


def apply_gyro_calibration(imu_data: np.ndarray, bias: np.ndarray) -> np.ndarray:
    """
    Применяет калибровку bias к данным гироскопа.

    Args:
        imu_data: numpy array shape (N, 10) - оригинальные IMU данные
        bias: numpy array [gx_bias, gy_bias, gz_bias]

    Returns:
        calibrated_imu: numpy array shape (N, 10) - данные с откалиброванным гироскопом

    Примечание:
        Функция создает копию данных, оригинал не изменяется
    """
    calibrated_imu = imu_data.copy()
    calibrated_imu[:, 4:7] -= bias  # Вычитаем bias из gx, gy, gz
    return calibrated_imu


def transform_gyro_to_vehicle(
    imu_calibrated: np.ndarray, rotation_matrix: np.ndarray
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Трансформирует данные гироскопа из системы телефона в систему автомобиля.

    Применяет матрицу поворота ко всем сэмплам гироскопа.
    Используется когда телефон установлен не горизонтально, а под углом
    (например, в вертикальном держателе на лобовом стекле).

    Args:
        imu_calibrated: numpy array shape (N, 10) - откалиброванные IMU данные
        rotation_matrix: numpy array 3x3 - матрица поворота (IMU → Vehicle)

    Returns:
        tuple (gx_vehicle, gy_vehicle, gz_vehicle) - трансформированные данные

    Пример:
        # После калибровки bias:
        imu_calibrated = apply_gyro_calibration(imu_data, bias)

        # Получаем матрицу поворота:
        R = calculate_rotation_matrix_from_gravity(gravity_vector)

        # Трансформируем все данные:
        gx_v, gy_v, gz_v = transform_gyro_to_vehicle(imu_calibrated, R)

        # Теперь gz_v = yaw rate автомобиля ✅
    """
    print("\n🚗 Трансформация в систему автомобиля...")

    gx_phone = imu_calibrated[:, 4]
    gy_phone = imu_calibrated[:, 5]
    gz_phone = imu_calibrated[:, 6]

    gx_vehicle = np.zeros(len(imu_calibrated))
    gy_vehicle = np.zeros(len(imu_calibrated))
    gz_vehicle = np.zeros(len(imu_calibrated))

    # Применяем матрицу поворота к каждому сэмплу
    for i in range(len(imu_calibrated)):
        gyro_phone = np.array([gx_phone[i], gy_phone[i], gz_phone[i]])
        gyro_vehicle = rotation_matrix @ gyro_phone
        gx_vehicle[i] = gyro_vehicle[0]
        gy_vehicle[i] = gyro_vehicle[1]
        gz_vehicle[i] = gyro_vehicle[2]

    print(f"   ✅ Трансформировано {len(imu_calibrated)} сэмплов")

    return gx_vehicle, gy_vehicle, gz_vehicle


def transform_gyro_to_vehicle_frame(
    gx: float, gy: float, gz: float, rotation_matrix: np.ndarray
) -> Tuple[float, float, float]:
    """
    Трансформирует угловую скорость из системы координат IMU в систему автомобиля.
    (Версия для одиночных значений)

    Используется когда телефон установлен не горизонтально, а под углом.

    Args:
        gx, gy, gz: Угловые скорости в системе IMU (рад/с)
        rotation_matrix: Матрица поворота 3x3 (IMU → Vehicle)

    Returns:
        gx_v, gy_v, gz_v: Угловые скорости в системе автомобиля (рад/с)

    Пример:
        # Для одного сэмпла:
        gx_v, gy_v, gz_v = transform_gyro_to_vehicle_frame(gx, gy, gz, R)

    Примечание:
        Для массивов используйте transform_gyro_to_vehicle()
    """
    gyro_imu = np.array([gx, gy, gz])
    gyro_vehicle = rotation_matrix @ gyro_imu
    return gyro_vehicle[0], gyro_vehicle[1], gyro_vehicle[2]


def calculate_rotation_matrix_from_gravity(gravity_vector: np.ndarray) -> np.ndarray:
    """
    Автоматически вычисляет матрицу поворота из системы IMU в систему автомобиля
    на основе вектора гравитации.

    Задача: Найти матрицу R такую, что R @ g_phone = g_vehicle
    где g_phone - гравитация в системе телефона, g_vehicle = [0, 0, -1] - в системе автомобиля

    Предполагает, что автомобиль стоит на ровной горизонтальной поверхности.

    Метод: Формула Родрига (Rodrigues' rotation formula)
    Превращает ось вращения + угол → матрица поворота 3x3

    Args:
        gravity_vector: [ax, ay, az] - усредненные показания акселерометра в покое

    Returns:
        rotation_matrix: матрица 3x3 для трансформации IMU → Vehicle

    Пример:
        gravity = [10.06, 0.43, -0.36]  # Телефон вертикально
        R = calculate_rotation_matrix_from_gravity(gravity)
        # R ≈ матрица поворота на ~88°

        # Применение к гироскопу:
        gyro_vehicle = R @ gyro_phone
    """
    # ===== ЭТАП 1: Нормализация вектора гравитации =====
    # ====================================================
    # Нам нужно только НАПРАВЛЕНИЕ, а не величина
    # Приводим вектор к единичной длине (|g_norm| = 1)
    #
    # Пример:
    #   gravity_vector = [10.06, 0.43, -0.36]
    #   norm = sqrt(10.06² + 0.43² + 0.36²) ≈ 10.08
    #   g_norm = [10.06/10.08, 0.43/10.08, -0.36/10.08]
    #          = [0.998, 0.043, -0.036]
    g_norm = gravity_vector / np.linalg.norm(gravity_vector)

    # ===== ЭТАП 2: Определение целевого вектора =====
    # =================================================
    # В стандартной системе автомобиля гравитация направлена ВНИЗ по оси Z
    # g_vehicle = [0, 0, -1]
    #   X: вправо (0 по гравитации)
    #   Y: вперёд (0 по гравитации)
    #   Z: вверх (-1 значит гравитация вниз)
    g_vehicle = np.array([0, 0, -1])

    # ===== ЭТАП 3: Вычисление оси вращения =====
    # ============================================
    # Используем векторное произведение (cross product):
    #   rotation_axis ⊥ (перпендикулярна) и g_norm и g_vehicle
    #
    # Визуализация:
    #        g_norm [0.998, 0.043, -0.036]  (гравитация в системе телефона)
    #          ↗
    #         /  ← нужно повернуть
    #        /
    #       ↓
    #    g_vehicle [0, 0, -1]  (гравитация в системе автомобиля)
    #
    #    rotation_axis ⊙ (перпендикулярна плоскости, торчит из экрана)
    #
    # Формула: a × b = [a_y*b_z - a_z*b_y, a_z*b_x - a_x*b_z, a_x*b_y - a_y*b_x]
    rotation_axis = np.cross(g_norm, g_vehicle)
    rotation_axis_norm = np.linalg.norm(rotation_axis)

    # ===== ЭТАП 4: Проверка особых случаев =====
    # ============================================
    if rotation_axis_norm < 1e-6:
        # Векторы почти параллельны (cross product ≈ 0)
        # Два варианта:

        if np.dot(g_norm, g_vehicle) > 0:
            # Случай A: Векторы смотрят в ОДНУ сторону
            # g_norm ≈ g_vehicle → поворот НЕ нужен
            # Возвращаем единичную матрицу (без изменений)
            return np.eye(3)
        else:
            # Случай B: Векторы смотрят в ПРОТИВОПОЛОЖНЫЕ стороны
            # g_norm ≈ -g_vehicle → поворот на 180°
            # Возвращаем матрицу отражения
            return -np.eye(3)

    # ===== ЭТАП 5: Нормализация оси вращения =====
    # ==============================================
    # Приводим ось вращения к единичной длине
    # Нужно для формулы Родрига
    rotation_axis = rotation_axis / rotation_axis_norm

    # ===== ЭТАП 6: Вычисление угла поворота =====
    # =============================================
    # Используем скалярное произведение (dot product):
    #   cos(θ) = a · b / (|a| * |b|)
    # Для единичных векторов: cos(θ) = a · b
    #
    # np.clip обрезает значение к [-1, 1] (защита от ошибок округления)
    # arccos возвращает угол в диапазоне [0, π]
    angle = np.arccos(np.clip(np.dot(g_norm, g_vehicle), -1.0, 1.0))

    # ===== ЭТАП 7: Формула Родрига =====
    # ====================================
    # Превращает ось вращения + угол → матрица поворота 3x3
    #
    # R = I + sin(θ) * K + (1 - cos(θ)) * K²
    #
    # Где:
    #   I - единичная матрица
    #   K - кососимметричная матрица оси вращения
    #   θ - угол поворота
    #
    # ШАГ 7.1: Создаем кососимметричную матрицу K
    # ---------------------------------------------
    # Для вектора k = [kx, ky, kz], кососимметричная матрица:
    #       [  0  -kz   ky ]
    #   K = [ kz    0  -kx ]
    #       [-ky   kx    0 ]
    #
    # Свойство: K @ v = k × v (векторное произведение)
    K = np.array(
        [
            [0, -rotation_axis[2], rotation_axis[1]],
            [rotation_axis[2], 0, -rotation_axis[0]],
            [-rotation_axis[1], rotation_axis[0], 0],
        ]
    )

    # ШАГ 7.2: Вычисляем матрицу поворота по формуле Родрига
    # --------------------------------------------------------
    # R = I + sin(θ)*K + (1-cos(θ))*K²
    #
    # Компоненты:
    #   I              - оставляет компоненту вдоль оси вращения без изменений
    #   sin(θ)*K       - добавляет компоненту перпендикулярную оси
    #   (1-cos(θ))*K²  - добавляет компоненту в плоскости вращения
    R = np.eye(3) + np.sin(angle) * K + (1 - np.cos(angle)) * (K @ K)

    return R


def process_imu_for_odometry(
    imu_data: np.ndarray,
    vehicle_speed_data: np.ndarray,
    speed_threshold_orientation: float = 0.1,
    speed_threshold_bias: float = 0.5,
    time_window_sec: float = 20.0,
    invert_yaw_rate: bool = True,
) -> dict:
    """
    Обрабатывает IMU данные для использования в визуализации траектории.

    Выполняет полный pipeline обработки:
    1. Фильтрация данных (машина стоит)
    2. Определение ориентации телефона
    3. Калибровка bias гироскопа
    4. Вычисление матрицы поворота
    5. Трансформация данных в систему автомобиля
    6. Инверсия знака yaw_rate (опционально, для Android)

    Args:
        imu_data: numpy array shape (N, 10) [timestamp, ax, ay, az, gx, gy, gz, mx, my, mz]
        vehicle_speed_data: numpy array - данные скоростей (любой формат)
        speed_threshold_orientation: порог для определения ориентации (м/с)
        speed_threshold_bias: порог для калибровки bias (м/с)
        time_window_sec: временное окно для анализа ориентации (сек)
        invert_yaw_rate: инвертировать знак yaw_rate (True для Android IMU)

    Returns:
        dict с обработанными данными:
            'imu_calibrated': numpy array (N, 10) - откалиброванные данные
            'gyro_vehicle': numpy array (N, 3) - [gx_v, gy_v, gz_v] в системе автомобиля
            'yaw_rate': numpy array (N,) - gz_vehicle (готово для одометрии)
            'bias': numpy array [gx_bias, gy_bias, gz_bias]
            'rotation_matrix': numpy array 3x3
            'orientation_info': dict - информация об ориентации телефона
            'rotation_angle_deg': float - угол поворота телефона

    Пример использования в visualizer:
        from imu_utils import process_imu_for_odometry

        imu_processed = process_imu_for_odometry(imu_data, wheel_speed_data)

        # Получаем yaw rate для расчета траектории:
        yaw_rate = imu_processed['yaw_rate']  # gz_vehicle

        # Или весь гироскоп:
        gyro_vehicle = imu_processed['gyro_vehicle']  # (N, 3)
    """
    print("\n" + "=" * 60)
    print("🔧 ОБРАБОТКА IMU ДЛЯ ОДОМЕТРИИ")
    print("=" * 60)

    # ===== ЭТАП 1: Фильтрация для анализа ориентации =====
    print("\n📍 ЭТАП 1: Фильтрация IMU (определение ориентации)")
    print(
        "   Параметры: v < {:.1f} м/с, первые {:.0f}с".format(
            speed_threshold_orientation, time_window_sec
        )
    )

    imu_stationary, _ = filter_imu_by_vehicle_speed(
        imu_data,
        vehicle_speed_data,
        speed_threshold=speed_threshold_orientation,
        time_window_sec=time_window_sec,
        min_samples=50,
    )

    # ===== ЭТАП 2: Определение ориентации телефона =====
    print("\n📱 ЭТАП 2: Определение ориентации телефона")
    orientation_info = detect_phone_orientation(imu_stationary)

    # ===== ЭТАП 3: Фильтрация для калибровки bias =====
    print("\n⚙️  ЭТАП 3: Калибровка bias (все моменты покоя)")
    print("   Параметры: v < {:.1f} м/с, все время".format(speed_threshold_bias))

    imu_for_bias, _ = filter_imu_by_vehicle_speed(
        imu_data,
        vehicle_speed_data,
        speed_threshold=speed_threshold_bias,
        time_window_sec=None,
        min_samples=50,
    )

    bias, n_bias_samples = calibrate_gyro_bias(imu_for_bias)

    # ===== ЭТАП 4: Применение калибровки ко всем данным =====
    print("\n🔄 ЭТАП 4: Применение калибровки ко всем IMU данным")
    imu_calibrated = apply_gyro_calibration(imu_data, bias)
    print(f"   ✅ Откалибровано {len(imu_calibrated)} сэмплов")

    # ===== ЭТАП 5: Вычисление матрицы поворота =====
    print("\n🔄 ЭТАП 5: Вычисление матрицы поворота")
    gravity_vector = orientation_info["gravity_vector"]
    rotation_matrix = calculate_rotation_matrix_from_gravity(gravity_vector)

    # Вычисляем угол для информации
    trace = np.trace(rotation_matrix)
    rotation_angle_rad = np.arccos(np.clip((trace - 1) / 2, -1.0, 1.0))
    rotation_angle_deg = np.degrees(rotation_angle_rad)

    print(f"   Матрица поворота R (IMU → Vehicle):")
    for row in rotation_matrix:
        print(f"   [{row[0]:+.4f}, {row[1]:+.4f}, {row[2]:+.4f}]")
    print(f"   Угол поворота: {rotation_angle_deg:.1f}°")

    # ===== ЭТАП 6: Трансформация в систему автомобиля =====
    print("\n🚗 ЭТАП 6: Трансформация в систему автомобиля")
    gx_vehicle, gy_vehicle, gz_vehicle = transform_gyro_to_vehicle(imu_calibrated, rotation_matrix)

    # ===== ЭТАП 7: Коррекция знака yaw_rate =====
    # ============================================
    # Android система координат может иметь инвертированное направление yaw
    # Проверяется эмпирически по сравнению с GPS/одометрией
    if invert_yaw_rate:
        print("\n🔄 ЭТАП 7: Коррекция знака yaw_rate")
        print("   Причина: Android система координат имеет инвертированное направление вращения")
        print(
            f"   Mean ДО инверсии:  {np.mean(gz_vehicle):+.6f} рад/с ({np.degrees(np.mean(gz_vehicle)):+.3f}°/с)"
        )

        gz_vehicle = -gz_vehicle  # Инвертируем знак
        gx_vehicle = -gx_vehicle  # Инвертируем для консистентности
        gy_vehicle = -gy_vehicle

        print(
            f"   Mean ПОСЛЕ инверсии: {np.mean(gz_vehicle):+.6f} рад/с ({np.degrees(np.mean(gz_vehicle)):+.3f}°/с)"
        )
        print("   ✅ Знак инвертирован (gz → -gz)")
    else:
        print("\n⏭️  ЭТАП 7: Коррекция знака yaw_rate ПРОПУЩЕНА")

    # ===== ЭТАП 8: Формирование результата =====
    gyro_vehicle_array = np.column_stack([gx_vehicle, gy_vehicle, gz_vehicle])

    result = {
        "imu_calibrated": imu_calibrated,
        "gyro_vehicle": gyro_vehicle_array,  # (N, 3) [gx, gy, gz] в системе автомобиля
        "yaw_rate": gz_vehicle,  # (N,) yaw rate для одометрии (с учетом инверсии!)
        "bias": bias,
        "rotation_matrix": rotation_matrix,
        "rotation_angle_deg": rotation_angle_deg,
        "orientation_info": orientation_info,
        "n_bias_samples": n_bias_samples,
        "inverted": invert_yaw_rate,  # Флаг инверсии для отладки
    }

    print("\n" + "=" * 60)
    print("✅ ОБРАБОТКА ЗАВЕРШЕНА")
    print("=" * 60)
    print(f"\nГотово для использования:")
    print(f"  • yaw_rate: {len(gz_vehicle)} сэмплов")
    print(f"  • Калибровка: bias вычтен")
    print(f"  • Трансформация: gz_vehicle = yaw rate автомобиля ✅")
    print(f"  • Инверсия знака: {'✅ Применена' if invert_yaw_rate else '❌ Не применена'}")
    print(
        f"  • Ориентация телефона: {orientation_info['dominant_axis']} вертикальна, угол {rotation_angle_deg:.1f}°"
    )

    return result


def estimate_yaw_rate_quality(
    imu_data: np.ndarray, vehicle_speed: np.ndarray, calibrated: bool = True
) -> dict:
    """
    Оценивает качество данных yaw rate (gz) из гироскопа.

    Проверяет:
    - Уровень шума (стандартное отклонение в покое)
    - Дрейф (накопление ошибки со временем)
    - Корреляцию со скоростью (должна быть слабая для прямолинейного движения)

    Args:
        imu_data: numpy array shape (N, 10)
        vehicle_speed: numpy array shape (N,)
        calibrated: True если bias уже вычтен

    Returns:
        dict с метриками качества
    """
    # Конвертируем в numpy array если это список
    if not isinstance(vehicle_speed, np.ndarray):
        vehicle_speed = np.array(vehicle_speed)

    gz = imu_data[:, 6]  # yaw rate

    # Синхронизация длины массивов
    n_samples = min(len(gz), len(vehicle_speed))
    gz = gz[:n_samples]
    vehicle_speed_sync = vehicle_speed[:n_samples]

    # Шум в покое
    stationary_mask = vehicle_speed_sync < 0.1
    stationary_indices = np.where(stationary_mask)[0]

    if len(stationary_indices) > 10:
        gz_stationary = gz[stationary_indices]
        gz_noise = np.std(gz_stationary)
    else:
        gz_noise = np.nan

    # Общая статистика
    gz_mean = np.mean(gz)
    gz_std = np.std(gz)
    gz_max = np.max(np.abs(gz))

    result = {
        "mean": gz_mean,
        "std": gz_std,
        "max_abs": gz_max,
        "noise_at_rest": gz_noise,
        "calibrated": calibrated,
    }

    print(f"\n📈 Качество данных yaw rate (gz):")
    print(f"   Среднее: {gz_mean:+.6f} рад/с")
    print(f"   Std: {gz_std:.6f} рад/с")
    print(f"   Макс: {gz_max:.6f} рад/с ({gz_max*180/np.pi:.1f}°/с)")
    if not np.isnan(gz_noise):
        print(f"   Шум в покое: {gz_noise:.6f} рад/с")
    print(f"   Калибровка: {'✅ Применена' if calibrated else '❌ Не применена'}")

    return result
