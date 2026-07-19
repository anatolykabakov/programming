#!/usr/bin/env python3
"""
Функции для визуализации траекторий автомобиля.
"""

import numpy as np
import matplotlib.pyplot as plt
import os
from typing import Optional


def plot_trajectory(
    x_odom,
    y_odom,
    x_gps=None,
    y_gps=None,
    x_imu=None,
    y_imu=None,
    x_fusion=None,
    y_fusion=None,
    x_gps_fusion=None,
    y_gps_fusion=None,
    x_ekf=None,
    y_ekf=None,
    output_dir="plots",
):
    """
    Строит график траектории движения автомобиля.
    Отображает одометрию, IMU (гироскоп), Fusion и GPS (если доступно).

    Args:
        x_odom: Список координат X по одометрии (steering)
        y_odom: Список координат Y по одометрии (steering)
        x_gps: Список координат X по GPS (опционально)
        y_gps: Список координат Y по GPS (опционально)
        x_imu: Список координат X по IMU (гироскоп) (опционально)
        y_imu: Список координат Y по IMU (гироскоп) (опционально)
        x_fusion: Список координат X по Fusion (IMU + Steering) (опционально)
        y_fusion: Список координат Y по Fusion (IMU + Steering) (опционально)
        x_gps_fusion: Список координат X по GPS Fusion (IMU + Steering + GPS) (опционально)
        y_gps_fusion: Список координат Y по GPS Fusion (IMU + Steering + GPS) (опционально)
        x_ekf: Список координат X по EKF (Extended Kalman Filter) (опционально)
        y_ekf: Список координат Y по EKF (Extended Kalman Filter) (опционально)
        output_dir: Директория для сохранения графиков
    """
    # Проверка на пустые данные
    if not x_odom or not y_odom or len(x_odom) == 0:
        print("Предупреждение: Нет данных одометрии для построения траектории")
        return

    # Создаем директорию для графиков
    os.makedirs(output_dir, exist_ok=True)

    # Настройка matplotlib
    plt.style.use("default")
    fig_size = (14, 10)

    # Преобразуем списки в numpy массивы
    x_odom_array = np.array(x_odom)
    y_odom_array = np.array(y_odom)

    # ===== СТАТИСТИКА ТРАЕКТОРИЙ =====
    print("\n" + "=" * 60)
    print("📊 СТАТИСТИКА ТРАЕКТОРИЙ:")
    print("=" * 60)

    print(f"\n🔵 Одометрия (Steering):")
    print(f"   Точек: {len(x_odom_array)}")
    print(
        f"   X: [{x_odom_array.min():.2f}, {x_odom_array.max():.2f}] м, range={x_odom_array.max()-x_odom_array.min():.2f} м"
    )
    print(
        f"   Y: [{y_odom_array.min():.2f}, {y_odom_array.max():.2f}] м, range={y_odom_array.max()-y_odom_array.min():.2f} м"
    )
    print(f"   Начало: ({x_odom_array[0]:.2f}, {y_odom_array[0]:.2f})")
    print(f"   Конец:  ({x_odom_array[-1]:.2f}, {y_odom_array[-1]:.2f})")
    distance_odom = np.sum(np.sqrt(np.diff(x_odom_array) ** 2 + np.diff(y_odom_array) ** 2))
    print(f"   Пройденное расстояние: {distance_odom:.2f} м")

    if x_imu is not None and y_imu is not None and len(x_imu) > 0:
        x_imu_array = np.array(x_imu)
        y_imu_array = np.array(y_imu)
        print(f"\n🟢 IMU (Гироскоп):")
        print(f"   Точек: {len(x_imu_array)}")
        print(
            f"   X: [{x_imu_array.min():.2f}, {x_imu_array.max():.2f}] м, range={x_imu_array.max()-x_imu_array.min():.2f} м"
        )
        print(
            f"   Y: [{y_imu_array.min():.2f}, {y_imu_array.max():.2f}] м, range={y_imu_array.max()-y_imu_array.min():.2f} м"
        )
        print(f"   Начало: ({x_imu_array[0]:.2f}, {y_imu_array[0]:.2f})")
        print(f"   Конец:  ({x_imu_array[-1]:.2f}, {y_imu_array[-1]:.2f})")
        distance_imu = np.sum(np.sqrt(np.diff(x_imu_array) ** 2 + np.diff(y_imu_array) ** 2))
        print(f"   Пройденное расстояние: {distance_imu:.2f} м")
        print(
            f"   Разница с одометрией: {distance_imu - distance_odom:.2f} м ({(distance_imu/distance_odom - 1)*100:.1f}%)"
        )

    if x_fusion is not None and y_fusion is not None and len(x_fusion) > 0:
        x_fusion_array = np.array(x_fusion)
        y_fusion_array = np.array(y_fusion)
        print(f"\n🟣 Fusion (IMU + Steering):")
        print(f"   Точек: {len(x_fusion_array)}")
        print(
            f"   X: [{x_fusion_array.min():.2f}, {x_fusion_array.max():.2f}] м, range={x_fusion_array.max()-x_fusion_array.min():.2f} м"
        )
        print(
            f"   Y: [{y_fusion_array.min():.2f}, {y_fusion_array.max():.2f}] м, range={y_fusion_array.max()-y_fusion_array.min():.2f} м"
        )
        print(f"   Начало: ({x_fusion_array[0]:.2f}, {y_fusion_array[0]:.2f})")
        print(f"   Конец:  ({x_fusion_array[-1]:.2f}, {y_fusion_array[-1]:.2f})")
        distance_fusion = np.sum(
            np.sqrt(np.diff(x_fusion_array) ** 2 + np.diff(y_fusion_array) ** 2)
        )
        print(f"   Пройденное расстояние: {distance_fusion:.2f} м")
        print(
            f"   Разница с одометрией: {distance_fusion - distance_odom:.2f} м ({(distance_fusion/distance_odom - 1)*100:.1f}%)"
        )

    if x_gps_fusion is not None and y_gps_fusion is not None and len(x_gps_fusion) > 0:
        x_gps_fusion_array = np.array(x_gps_fusion)
        y_gps_fusion_array = np.array(y_gps_fusion)
        print(f"\n🟠 GPS Fusion (IMU + Steering + GPS коррекция):")
        print(f"   Точек: {len(x_gps_fusion_array)}")
        print(
            f"   X: [{x_gps_fusion_array.min():.2f}, {x_gps_fusion_array.max():.2f}] м, range={x_gps_fusion_array.max()-x_gps_fusion_array.min():.2f} м"
        )
        print(
            f"   Y: [{y_gps_fusion_array.min():.2f}, {y_gps_fusion_array.max():.2f}] м, range={y_gps_fusion_array.max()-y_gps_fusion_array.min():.2f} м"
        )
        print(f"   Начало: ({x_gps_fusion_array[0]:.2f}, {y_gps_fusion_array[0]:.2f})")
        print(f"   Конец:  ({x_gps_fusion_array[-1]:.2f}, {y_gps_fusion_array[-1]:.2f})")
        distance_gps_fusion = np.sum(
            np.sqrt(np.diff(x_gps_fusion_array) ** 2 + np.diff(y_gps_fusion_array) ** 2)
        )
        print(f"   Пройденное расстояние: {distance_gps_fusion:.2f} м")
        print(
            f"   Разница с одометрией: {distance_gps_fusion - distance_odom:.2f} м ({(distance_gps_fusion/distance_odom - 1)*100:.1f}%)"
        )

    if x_ekf is not None and y_ekf is not None and len(x_ekf) > 0:
        x_ekf_array = np.array(x_ekf)
        y_ekf_array = np.array(y_ekf)
        print(f"\n⭐ EKF (Extended Kalman Filter):")
        print(f"   Точек: {len(x_ekf_array)}")
        print(
            f"   X: [{x_ekf_array.min():.2f}, {x_ekf_array.max():.2f}] м, range={x_ekf_array.max()-x_ekf_array.min():.2f} м"
        )
        print(
            f"   Y: [{y_ekf_array.min():.2f}, {y_ekf_array.max():.2f}] м, range={y_ekf_array.max()-y_ekf_array.min():.2f} м"
        )
        print(f"   Начало: ({x_ekf_array[0]:.2f}, {y_ekf_array[0]:.2f})")
        print(f"   Конец:  ({x_ekf_array[-1]:.2f}, {y_ekf_array[-1]:.2f})")
        distance_ekf = np.sum(np.sqrt(np.diff(x_ekf_array) ** 2 + np.diff(y_ekf_array) ** 2))
        print(f"   Пройденное расстояние: {distance_ekf:.2f} м")
        print(
            f"   Разница с одометрией: {distance_ekf - distance_odom:.2f} м ({(distance_ekf/distance_odom - 1)*100:.1f}%)"
        )

    if x_gps is not None and y_gps is not None and len(x_gps) > 0:
        x_gps_array = np.array(x_gps)
        y_gps_array = np.array(y_gps)
        print(f"\n🔴 GPS (Ground Truth):")
        print(f"   Точек: {len(x_gps_array)}")
        print(
            f"   X: [{x_gps_array.min():.2f}, {x_gps_array.max():.2f}] м, range={x_gps_array.max()-x_gps_array.min():.2f} м"
        )
        print(
            f"   Y: [{y_gps_array.min():.2f}, {y_gps_array.max():.2f}] м, range={y_gps_array.max()-y_gps_array.min():.2f} м"
        )
        print(f"   Начало: ({x_gps_array[0]:.2f}, {y_gps_array[0]:.2f})")
        print(f"   Конец:  ({x_gps_array[-1]:.2f}, {y_gps_array[-1]:.2f})")
        distance_gps = np.sum(np.sqrt(np.diff(x_gps_array) ** 2 + np.diff(y_gps_array) ** 2))
        print(f"   Пройденное расстояние: {distance_gps:.2f} м")

    print("\n" + "=" * 60)

    # Создаем фигуру
    plt.figure(figsize=fig_size)

    # Траектория по одометрии (steering)
    plt.plot(
        x_odom_array,
        y_odom_array,
        "b-",
        linewidth=2,
        alpha=0.7,
        label="Одометрия (Steering)",
    )
    plt.scatter(
        x_odom_array[0],
        y_odom_array[0],
        color="green",
        s=150,
        label="Начало",
        zorder=5,
        marker="o",
    )
    plt.scatter(
        x_odom_array[-1],
        y_odom_array[-1],
        color="red",
        s=150,
        label="Конец",
        zorder=5,
        marker="s",
    )

    # Траектория по IMU (гироскоп) (если есть)
    if x_imu is not None and y_imu is not None and len(x_imu) > 0:
        x_imu_array = np.array(x_imu)
        y_imu_array = np.array(y_imu)
        plt.plot(x_imu_array, y_imu_array, "g-", linewidth=2, alpha=0.6, label="IMU (Гироскоп)")
        plt.scatter(x_imu_array[0], y_imu_array[0], color="green", s=100, zorder=4, marker="D")
        plt.scatter(x_imu_array[-1], y_imu_array[-1], color="red", s=100, zorder=4, marker="D")

    # Траектория по Fusion (IMU + Steering) (если есть)
    if x_fusion is not None and y_fusion is not None and len(x_fusion) > 0:
        x_fusion_array = np.array(x_fusion)
        y_fusion_array = np.array(y_fusion)
        plt.plot(
            x_fusion_array,
            y_fusion_array,
            "m-",
            linewidth=2,
            alpha=0.6,
            label="Fusion (IMU+Steering)",
            zorder=2,
        )
        plt.scatter(
            x_fusion_array[0],
            y_fusion_array[0],
            color="green",
            s=100,
            zorder=4,
            marker="*",
        )
        plt.scatter(
            x_fusion_array[-1],
            y_fusion_array[-1],
            color="red",
            s=100,
            zorder=4,
            marker="*",
        )

    # Траектория по GPS Fusion (IMU + Steering + GPS коррекция) (если есть)
    if x_gps_fusion is not None and y_gps_fusion is not None and len(x_gps_fusion) > 0:
        x_gps_fusion_array = np.array(x_gps_fusion)
        y_gps_fusion_array = np.array(y_gps_fusion)
        plt.plot(
            x_gps_fusion_array,
            y_gps_fusion_array,
            "orange",
            linewidth=2,
            alpha=0.7,
            label="GPS Fusion (IMU+Str+GPS)",
            zorder=3,
        )
        plt.scatter(
            x_gps_fusion_array[0],
            y_gps_fusion_array[0],
            color="green",
            s=120,
            zorder=5,
            marker="P",
        )
        plt.scatter(
            x_gps_fusion_array[-1],
            y_gps_fusion_array[-1],
            color="red",
            s=120,
            zorder=5,
            marker="P",
        )

    # Траектория по EKF (Extended Kalman Filter) (если есть) ⭐ ЛУЧШИЙ
    if x_ekf is not None and y_ekf is not None and len(x_ekf) > 0:
        x_ekf_array = np.array(x_ekf)
        y_ekf_array = np.array(y_ekf)
        plt.plot(
            x_ekf_array,
            y_ekf_array,
            color="cyan",
            linewidth=3.5,
            alpha=1.0,
            label="EKF (Kalman Filter)",
            zorder=6,
        )
        plt.scatter(
            x_ekf_array[0],
            y_ekf_array[0],
            color="cyan",
            s=180,
            zorder=7,
            marker="*",
            edgecolors="black",
            linewidths=2,
        )
        plt.scatter(
            x_ekf_array[-1],
            y_ekf_array[-1],
            color="cyan",
            s=180,
            zorder=7,
            marker="*",
            edgecolors="black",
            linewidths=2,
        )

    # Траектория по GPS (если есть)
    if x_gps is not None and y_gps is not None and len(x_gps) > 0:
        x_gps_array = np.array(x_gps)
        y_gps_array = np.array(y_gps)
        plt.plot(
            x_gps_array,
            y_gps_array,
            "r--",
            linewidth=2,
            alpha=0.6,
            label="GPS (Ground Truth)",
        )
        plt.scatter(x_gps_array[0], y_gps_array[0], color="green", s=100, zorder=4, marker="^")
        plt.scatter(x_gps_array[-1], y_gps_array[-1], color="red", s=100, zorder=4, marker="v")

    plt.xlabel("X (м)", fontsize=12)
    plt.ylabel("Y (м)", fontsize=12)
    plt.title("Траектория движения автомобиля", fontsize=14, fontweight="bold")
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3, linestyle="--")
    plt.axis("equal")

    # Сохранение графика
    output_path = os.path.join(output_dir, "trajectory.png")
    plt.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"График сохранен: {output_path}")
