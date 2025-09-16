#!/usr/bin/env python3
"""
Главный скрипт визуализации и анализа логов ADAS системы.

Поддерживаемые функции:
- Парсинг логов (IMU, GPS, CAN, Camera)
- Расчет траекторий (Одометрия, IMU, Fusion, GPS)
- Визуализация траекторий
- Анализ данных

Использование:
    python3 visualizer.py --input /path/to/logs --plot-trajectory
"""

import numpy as np
import matplotlib

matplotlib.use("Agg")  # Использовать бэкенд без GUI
import argparse
import os

# Импорты модулей
from log_parser import LogParser
from gps_utils import gps_to_local_coords, calculate_initial_heading_from_gps
from imu_utils import process_imu_for_odometry
from trajectory_calculators import (
    calculate_trajectory,
    calculate_trajectory_imu,
    calculate_trajectory_fusion,
    calculate_trajectory_gps_fusion,
    calculate_trajectory_ekf,
)
from plotting import plot_trajectory


def build_trajectories(log_parser, output_dir):
    """Построение всех типов траекторий"""
    wheel = log_parser.get_wheel_speed()
    steering = log_parser.get_steering()
    gear = log_parser.get_gear()
    gps_data = log_parser.get_gps()

    # Расчет начальной ориентации по GPS
    initial_yaw = 0.0
    if gps_data is not None and len(gps_data) >= 2:
        initial_yaw = calculate_initial_heading_from_gps(gps_data, n_points=5)
        print(
            f"\nНачальная ориентация по GPS: {np.degrees(initial_yaw):.1f}° (yaw={initial_yaw:.3f} рад)"
        )

    # Конвертация GPS в локальные координаты
    x_gps, y_gps = None, None
    if gps_data is not None and len(gps_data) > 0:
        print("Конвертация GPS координат в локальную систему...")
        x_gps, y_gps = gps_to_local_coords(gps_data, origin_idx=0)
        print(f"GPS траектория: {len(x_gps)} точек")

    # Обработка IMU данных для получения yaw rate
    x_imu, y_imu, x_fusion, y_fusion, x_gps_fusion, y_gps_fusion, x_ekf, y_ekf = (
        None,
        None,
        None,
        None,
        None,
        None,
        None,
        None,
    )
    imu_data = log_parser.get_imu()
    if (
        imu_data is not None
        and len(imu_data) > 0
        and wheel
        and len(wheel) > 0
        and gps_data is not None
    ):
        print("\n" + "=" * 60)
        print("Обработка IMU данных...")
        print("=" * 60)

        try:
            # Обработка IMU (калибровка + трансформация + коррекция знака)
            imu_processed = process_imu_for_odometry(
                imu_data,
                np.array(wheel),  # [[timestamp, fl, fr, rl, rr], ...]
                speed_threshold_orientation=0.1,
                speed_threshold_bias=0.5,
                time_window_sec=20.0,
                invert_yaw_rate=True,  # Инвертируем для Android IMU
            )

            # Расчет траектории с IMU
            print("\nРасчет траектории по IMU (гироскоп)...")
            x_imu, y_imu = calculate_trajectory_imu(
                wheel,
                imu_processed["yaw_rate"],
                imu_processed["imu_calibrated"][:, 0],  # timestamps
                initial_yaw=initial_yaw,
            )
            print(f"Траектория IMU: {len(x_imu)} точек")

            # Расчет траектории с EKF (Extended Kalman Filter)
            print("\n" + "=" * 60)
            print("Расчет траектории EKF (Extended Kalman Filter)...")
            print("=" * 60)
            x_ekf, y_ekf, ekf = calculate_trajectory_ekf(
                wheel,
                steering,
                gear,
                imu_processed["yaw_rate"],
                imu_processed["imu_calibrated"][:, 0],
                gps_data,
                wheelbase=2.636,
                initial_yaw=initial_yaw,
                alpha_imu=0.7,  # 70% IMU в prediction
                gps_update_interval=1.0,  # GPS каждую секунду
                imu_update_interval=0.01,  # IMU каждые 10мс
            )
            print(f"Траектория EKF: {len(x_ekf)} точек")

        except Exception as e:
            print(f"⚠️  Warning: Не удалось обработать IMU данные: {e}")
            import traceback

            traceback.print_exc()
            x_imu, y_imu = None, None
            x_fusion, y_fusion = None, None
            x_gps_fusion, y_gps_fusion = None, None
            x_ekf, y_ekf = None, None

    # Расчет траектории по одометрии с начальной ориентацией
    if wheel and len(wheel) > 0:
        print("\nРасчет траектории по одометрии (steering)...")
        x_odom, y_odom = calculate_trajectory(
            wheel, steering, gear, wheelbase=2.636, initial_yaw=initial_yaw
        )

        # Построить график с всеми траекториями
        plot_trajectory(
            x_odom,
            y_odom,
            x_gps,
            y_gps,
            x_imu,
            y_imu,
            x_fusion,
            y_fusion,
            x_gps_fusion,
            y_gps_fusion,
            x_ekf,
            y_ekf,
            output_dir,
        )
        print(f"\n✅ Траектория сохранена в {output_dir}/trajectory.png")
    else:
        print("Предупреждение: Нет данных одометрии для построения траектории")


def main():
    parser = argparse.ArgumentParser(description="Визуализация и анализ логов ADAS системы")
    parser.add_argument(
        "--input",
        "-i",
        required=True,
        help="Путь к директории с логами или ZIP архиву",
    )
    parser.add_argument(
        "--output", "-o", default="plots", help="Директория для сохранения графиков"
    )
    parser.add_argument(
        "--no-show", action="store_true", help="Не показывать графики, только сохранять"
    )
    parser.add_argument(
        "--plot-trajectory", action="store_true", help="Построить траекторию по одометрии"
    )

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Ошибка: Путь {args.input} не найден")
        return 1

    log_parser = LogParser(args.input)
    log_parser.parse_all()

    # Вывести сводку
    summary = log_parser.get_summary()
    print("\n" + "=" * 60)
    print("СВОДКА ПО ДАННЫМ:")
    print("=" * 60)
    print(f"IMU записей: {summary['imu_records']}")
    print(f"GPS записей: {summary['gps_records']}")
    print(f"CAN фреймов: {summary['can_frames']}")
    print(f"Одометрия:")
    print(f"  - Скорости колес: {summary['wheel_speed_records']}")
    print(f"  - Углы руля: {summary['steering_records']}")
    print(f"  - Передачи: {summary['gear_records']}")
    print(f"Изображений камеры: {summary['camera_images']}")
    print(f"Параметры камеры: {'Да' if summary['has_intrinsics'] else 'Нет'}")
    if "duration_seconds" in summary:
        print(f"Длительность сессии: {summary['duration_seconds']:.1f} сек")
    print("=" * 60 + "\n")

    # Построить траекторию если запрошено
    if args.plot_trajectory:
        build_trajectories(log_parser, args.output)

    # Пример: показать первое изображение
    images = log_parser.get_images()
    if images and len(images) > 0:
        first_image_path = images[0]
        img = log_parser.read_camera_image(first_image_path)
        if img is not None:
            print(f"\nПервое изображение: {first_image_path}")
            print(f"Размер: {img.shape}")

    return 0


if __name__ == "__main__":
    exit(main())
