#!/usr/bin/env python3
"""
Экспорт логов ADAS в форматы, совместимые с PlotJuggler.

PlotJuggler - профессиональный инструмент для визуализации временных рядов:
- Интерактивные графики с синхронизацией
- Множество встроенных функций
- Поддержка CSV, ROS bags, JSON
- Timeline с воспроизведением

Этот скрипт конвертирует наши логи в CSV формат для PlotJuggler.

Использование:
    python3 export_to_plotjuggler.py --input /path/to/logs --output data.csv
    plotjuggler data.csv
"""

import numpy as np
import argparse
import os
import csv
from pathlib import Path

from log_parser import LogParser
from gps_utils import gps_to_local_coords, calculate_initial_heading_from_gps
from imu_utils import process_imu_for_odometry
from trajectory_calculators import (
    calculate_trajectory,
    calculate_trajectory_imu,
    calculate_trajectory_fusion,
    calculate_trajectory_ekf,
)


def export_to_csv(log_parser, output_file, include_trajectories=True):
    """
    Экспорт данных в CSV формат для PlotJuggler.

    Формат CSV:
    timestamp, imu_ax, imu_ay, imu_az, imu_gx, imu_gy, imu_gz,
    gps_lat, gps_lon, gps_alt, wheel_fl, wheel_fr, wheel_rl, wheel_rr,
    steering_angle, gear, traj_odom_x, traj_odom_y, traj_gps_x, traj_gps_y, ...
    """

    print(f"\n{'='*60}")
    print("Экспорт данных для PlotJuggler")
    print(f"{'='*60}\n")

    # Получить все данные
    imu_data = log_parser.get_imu()
    gps_data = log_parser.get_gps()
    wheel_data = log_parser.get_wheel_speed()
    steering_data = log_parser.get_steering()
    gear_data = log_parser.get_gear()

    # Конвертировать в numpy arrays
    imu_array = np.array(imu_data) if imu_data is not None and len(imu_data) > 0 else None
    gps_array = np.array(gps_data) if gps_data is not None and len(gps_data) > 0 else None
    wheel_array = np.array(wheel_data) if wheel_data is not None and len(wheel_data) > 0 else None
    steering_array = (
        np.array(steering_data) if steering_data is not None and len(steering_data) > 0 else None
    )
    gear_array = np.array(gear_data) if gear_data is not None and len(gear_data) > 0 else None

    if wheel_array is None or len(wheel_array) == 0:
        print("❌ Нет данных одометрии для экспорта")
        return False

    # Базовые timestamps из одометрии
    timestamps = wheel_array[:, 0]
    n_samples = len(timestamps)

    # Конвертировать в относительное время (от 0)
    t0 = timestamps[0]
    timestamps_relative = (timestamps - t0) / 1e3  # Миллисекунды → секунды, от 0

    print(f"Базовые данные:")
    print(f"  • Timestamps: {n_samples} сэмплов")
    print(
        f"  • Длительность: {timestamps_relative[-1]:.1f} сек ({timestamps_relative[-1]/60:.1f} мин)"
    )
    print(f"  • Время от 0.000 до {timestamps_relative[-1]:.3f} сек")

    # Рассчитать траектории если запрошено
    trajectories = {}
    if include_trajectories:
        print(f"\nРасчет траекторий...")

        # Начальная ориентация
        initial_yaw = 0.0
        if gps_array is not None and len(gps_array) >= 2:
            initial_yaw = calculate_initial_heading_from_gps(gps_array, n_points=5)
            print(f"  Начальная ориентация: {np.degrees(initial_yaw):.1f}°")

        # GPS траектория
        if gps_array is not None and len(gps_array) > 0:
            x_gps, y_gps = gps_to_local_coords(gps_array, origin_idx=0)
            trajectories["gps"] = (x_gps, y_gps)
            print(f"  ✅ GPS: {len(x_gps)} точек")

        # Одометрия
        x_odom, y_odom = calculate_trajectory(
            wheel_array, steering_array, gear_array, wheelbase=2.636, initial_yaw=initial_yaw
        )
        trajectories["odom"] = (x_odom, y_odom)
        print(f"  ✅ Odometry: {len(x_odom)} точек")

        # IMU траектории
        if imu_array is not None and len(imu_array) > 0:
            try:
                imu_processed = process_imu_for_odometry(
                    imu_array,
                    wheel_array,
                    speed_threshold_orientation=0.1,
                    speed_threshold_bias=0.5,
                    time_window_sec=20.0,
                    invert_yaw_rate=True,
                )

                x_imu, y_imu = calculate_trajectory_imu(
                    wheel_array,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    initial_yaw=initial_yaw,
                )
                trajectories["imu"] = (x_imu, y_imu)
                print(f"  ✅ IMU: {len(x_imu)} точек")

                x_fusion, y_fusion = calculate_trajectory_fusion(
                    wheel_array,
                    steering_array,
                    gear_array,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    wheelbase=2.636,
                    initial_yaw=initial_yaw,
                    alpha=0.7,
                )
                trajectories["fusion"] = (x_fusion, y_fusion)
                print(f"  ✅ Fusion: {len(x_fusion)} точек")

                x_ekf, y_ekf, _ = calculate_trajectory_ekf(
                    wheel_array,
                    steering_array,
                    gear_array,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    gps_array,
                    wheelbase=2.636,
                    initial_yaw=initial_yaw,
                )
                trajectories["ekf"] = (x_ekf, y_ekf)
                print(f"  ✅ EKF: {len(x_ekf)} точек")

            except Exception as e:
                print(f"  ⚠️  Ошибка при расчете IMU траекторий: {e}")

    # Создать CSV файл
    print(f"\nЗапись в CSV: {output_file}")

    with open(output_file, "w", newline="") as csvfile:
        # Определить колонки
        fieldnames = ["timestamp_sec"]

        # IMU колонки
        if imu_array is not None:
            fieldnames.extend(
                [
                    "imu_accel_x",
                    "imu_accel_y",
                    "imu_accel_z",
                    "imu_gyro_x",
                    "imu_gyro_y",
                    "imu_gyro_z",
                    "imu_mag_x",
                    "imu_mag_y",
                    "imu_mag_z",
                ]
            )

        # GPS колонки
        if gps_array is not None:
            fieldnames.extend(["gps_lat", "gps_lon", "gps_alt", "gps_speed"])

        # Одометрия
        fieldnames.extend(["wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr", "wheel_avg_speed"])

        # Руль и передача
        if steering_array is not None:
            fieldnames.append("steering_angle")
        if gear_array is not None:
            fieldnames.append("gear")

        # Траектории
        if include_trajectories:
            for traj_name in ["odom", "gps", "imu", "fusion", "ekf"]:
                if traj_name in trajectories:
                    fieldnames.extend([f"traj_{traj_name}_x", f"traj_{traj_name}_y"])

        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        # Интерполировать все данные на базовый timeline
        for i, ts in enumerate(timestamps):
            # Форматировать время с фиксированной точностью (без научной нотации)
            row = {"timestamp_sec": f"{timestamps_relative[i]:.6f}"}  # 6 знаков после запятой

            # IMU данные (интерполяция)
            if imu_array is not None:
                imu_idx = np.searchsorted(imu_array[:, 0], ts)
                if 0 < imu_idx < len(imu_array):
                    imu = imu_array[imu_idx]
                    row.update(
                        {
                            "imu_accel_x": imu[1],
                            "imu_accel_y": imu[2],
                            "imu_accel_z": imu[3],
                            "imu_gyro_x": imu[4],
                            "imu_gyro_y": imu[5],
                            "imu_gyro_z": imu[6],
                            "imu_mag_x": imu[7],
                            "imu_mag_y": imu[8],
                            "imu_mag_z": imu[9],
                        }
                    )

            # GPS данные
            if gps_array is not None:
                gps_idx = np.searchsorted(gps_array[:, 0], ts)
                if 0 < gps_idx < len(gps_array):
                    gps = gps_array[gps_idx]
                    row.update(
                        {
                            "gps_lat": gps[1],
                            "gps_lon": gps[2],
                            "gps_alt": gps[3],
                            "gps_speed": gps[4] if len(gps) > 4 else 0.0,
                        }
                    )

            # Одометрия
            wheel = wheel_array[i]
            avg_speed = (wheel[1] + wheel[2] + wheel[3] + wheel[4]) / 4.0
            row.update(
                {
                    "wheel_fl": wheel[1],
                    "wheel_fr": wheel[2],
                    "wheel_rl": wheel[3],
                    "wheel_rr": wheel[4],
                    "wheel_avg_speed": avg_speed,
                }
            )

            # Руль
            if steering_array is not None:
                steer_idx = np.searchsorted(steering_array[:, 0], ts)
                if 0 < steer_idx < len(steering_array):
                    row["steering_angle"] = steering_array[steer_idx, 1]

            # Передача
            if gear_array is not None:
                gear_idx = np.searchsorted(gear_array[:, 0], ts)
                if 0 < gear_idx < len(gear_array):
                    gear_value = str(gear_array[gear_idx, 1]).upper()
                    # Конвертировать в число для PlotJuggler
                    gear_num = {
                        "PARK": 0,
                        "P": 0,
                        "REVERSE": 1,
                        "R": 1,
                        "NEUTRAL": 2,
                        "N": 2,
                        "DRIVE": 3,
                        "D": 3,
                    }.get(gear_value, -1)
                    row["gear"] = gear_num

            # Траектории
            if include_trajectories:
                for traj_name, (x, y) in trajectories.items():
                    if i < len(x):
                        row[f"traj_{traj_name}_x"] = x[i]
                        row[f"traj_{traj_name}_y"] = y[i]

            writer.writerow(row)

            # Прогресс
            if i % 5000 == 0:
                print(f"  Экспортировано {i}/{n_samples} записей...")

    print(f"\n✅ Экспорт завершен!")
    print(f"📊 Записано {n_samples} строк в {output_file}")
    print(f"📦 Размер файла: {os.path.getsize(output_file) / 1024 / 1024:.1f} MB")

    return True


def print_plotjuggler_instructions(csv_file):
    """Вывести инструкции по использованию PlotJuggler"""
    print(f"\n{'='*60}")
    print("🚀 КАК ИСПОЛЬЗОВАТЬ С PLOTJUGGLER")
    print(f"{'='*60}\n")

    print("1️⃣  Установка PlotJuggler (если не установлен):")
    print("   Ubuntu/Debian:")
    print("     sudo apt install plotjuggler")
    print("   или")
    print("     sudo snap install plotjuggler")
    print("   Или скачать: https://github.com/facontidavide/PlotJuggler\n")

    print("2️⃣  Запуск PlotJuggler:")
    print(f"   plotjuggler {csv_file}\n")

    print("3️⃣  В PlotJuggler:")
    print("   • Выберите 'CSV' в диалоге импорта")
    print("   • Используйте 'timestamp_sec' как временную ось")
    print("   • Перетащите графики из списка слева на панели справа\n")

    print("4️⃣  Рекомендуемые графики для анализа:\n")
    print("   📊 IMU Analysis:")
    print("      - imu_accel_x, imu_accel_y, imu_accel_z (ускорения)")
    print("      - imu_gyro_x, imu_gyro_y, imu_gyro_z (угловые скорости)")
    print("      - imu_mag_x, imu_mag_y, imu_mag_z (магнитометр)\n")

    print("   🌍 GPS:")
    print("      - gps_lat, gps_lon (координаты)")
    print("      - gps_alt (высота)")
    print("      - gps_speed (скорость)\n")

    print("   🚗 Одометрия:")
    print("      - wheel_fl, wheel_fr, wheel_rl, wheel_rr (скорости колес)")
    print("      - wheel_avg_speed (средняя скорость)")
    print("      - steering_angle (угол руля)")
    print("      - gear (передача: 0=P, 1=R, 2=N, 3=D)\n")

    print("   📍 Траектории (XY plot):")
    print("      - traj_odom_x vs traj_odom_y (одометрия)")
    print("      - traj_gps_x vs traj_gps_y (GPS)")
    print("      - traj_imu_x vs traj_imu_y (IMU)")
    print("      - traj_fusion_x vs traj_fusion_y (Fusion)")
    print("      - traj_ekf_x vs traj_ekf_y (EKF - лучший результат)\n")

    print("5️⃣  Фичи PlotJuggler:")
    print("   ✅ Синхронизация всех графиков по времени")
    print("   ✅ Zoom/Pan любого графика")
    print("   ✅ Timeline с воспроизведением")
    print("   ✅ Фильтрация и математические операции")
    print("   ✅ Экспорт графиков в PNG/PDF")
    print("   ✅ Создание custom layouts\n")

    print("6️⃣  Пример создания XY plot (траектория):")
    print("   • Создайте новый plot (кнопка '+' вверху)")
    print("   • Выберите тип 'XY Plot'")
    print("   • X axis: traj_ekf_x")
    print("   • Y axis: traj_ekf_y")
    print("   • Результат: 2D траектория автомобиля\n")

    print(f"{'='*60}\n")


def main():
    parser = argparse.ArgumentParser(description="Экспорт ADAS логов для PlotJuggler")
    parser.add_argument("--input", "-i", required=True, help="Путь к директории с логами")
    parser.add_argument(
        "--output", "-o", default="adas_data_plotjuggler.csv", help="Имя выходного CSV файла"
    )
    parser.add_argument(
        "--no-trajectories",
        action="store_true",
        help="Не включать рассчитанные траектории (быстрее)",
    )

    args = parser.parse_args()

    # Проверка входных данных
    if not os.path.exists(args.input):
        print(f"❌ Ошибка: путь {args.input} не найден")
        return 1

    # Парсинг логов
    print("Парсинг логов...")
    log_parser = LogParser(args.input)
    log_parser.parse_all()

    summary = log_parser.get_summary()
    print(f"\n📊 Сводка:")
    print(f"  • IMU: {summary['imu_records']} записей")
    print(f"  • GPS: {summary['gps_records']} записей")
    print(f"  • Wheel speeds: {summary['wheel_speed_records']} записей")
    print(f"  • Steering: {summary['steering_records']} записей")
    print(f"  • Gear: {summary['gear_records']} записей")
    print(f"  • Images: {summary['camera_images']} кадров")

    # Экспорт
    include_traj = not args.no_trajectories
    success = export_to_csv(log_parser, args.output, include_trajectories=include_traj)

    if success:
        # Вывести инструкции
        print_plotjuggler_instructions(args.output)
        return 0
    else:
        return 1


if __name__ == "__main__":
    exit(main())
