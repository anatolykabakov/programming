#!/usr/bin/env python3
"""
Модель кинематики автомобиля для расчета траектории.
"""

import numpy as np
from typing import Tuple


def normalize_angle(angle):
    """
    Нормализует угол к диапазону [-π, π].

    Args:
        angle: Угол в радианах (может быть массивом)

    Returns:
        Нормализованный угол в диапазоне [-π, π]
    """
    while np.any(angle > np.pi):
        angle = np.where(angle > np.pi, angle - 2.0 * np.pi, angle)
    while np.any(angle < -np.pi):
        angle = np.where(angle < -np.pi, angle + 2.0 * np.pi, angle)
    return angle


class VehicleModel:
    """
    Модель кинематики автомобиля (bicycle model).
    Вычисляет траекторию на основе скоростей колес, угла руля и передачи.
    """

    def __init__(self, wheelbase: float = 2.636, initial_yaw: float = 0.0):
        """
        Инициализация модели автомобиля.

        Args:
            wheelbase: Колесная база (расстояние между осями) в метрах
            initial_yaw: Начальная ориентация в радианах (0 = восток, π/2 = север)
        """
        self.wheelbase = wheelbase
        self.min_speed_threshold = 0.01  # м/с
        self.max_steering_angle = 32.72  # градусы
        self.max_steer_raw_value = 400
        self.unit_to_degrees = self.max_steering_angle / self.max_steer_raw_value

        # Состояние автомобиля
        self.x = 0.0
        self.y = 0.0
        self.yaw = initial_yaw
        self.initial_yaw = initial_yaw  # Сохранить для reset()

    def reset(self):
        """Сброс состояния автомобиля к начальным значениям"""
        self.x = 0.0
        self.y = 0.0
        self.yaw = self.initial_yaw

    def update(
        self,
        vr: float,
        vl: float,
        hr: float,
        hl: float,
        steering_angle_abs: float,
        steering_angle_sign: float,
        gear_name: str,
        dt: float,
    ) -> Tuple[float, float, float]:
        """
        Обновляет состояние автомобиля на основе данных одометрии.

        Args:
            vr, vl, hr, hl: Скорости колес в км/ч
            steering_angle_abs: |угол передних колёс| в «сырых» единицах
                (road-wheel deg / unit_to_degrees). Не угол руля LWI!
            steering_angle_sign: Знак угла (0=влево, 1=вправо)
            gear_name: Название передачи
            dt: Временной шаг в секундах

        Returns:
            Tuple (x, y, yaw) - текущая позиция и ориентация
        """
        # Вычисление угла поворота
        steering_angle_deg = steering_angle_abs * self.unit_to_degrees
        if steering_angle_sign != 0:
            steering_angle_deg = -steering_angle_deg

        steering_angle_deg = np.clip(
            steering_angle_deg, -self.max_steering_angle, self.max_steering_angle
        )
        steering_angle_rad = np.radians(steering_angle_deg)

        # Скорость автомобиля
        v_vehicle_kmh = (vr + vl + hr + hl) / 4.0
        v_vehicle_ms = v_vehicle_kmh / 3.6  # км/ч -> м/с

        # Направление (реверс)
        direction_multiplier = -1.0 if gear_name == "REVERSE" else 1.0
        v_vehicle_ms *= direction_multiplier

        # Пропустить если скорость слишком мала
        if abs(v_vehicle_ms) <= self.min_speed_threshold:
            return self.x, self.y, self.yaw

        # Угловая скорость (bicycle model)
        if abs(steering_angle_rad) >= 0.001:
            omega = v_vehicle_ms * np.tan(steering_angle_rad) / self.wheelbase
        else:
            omega = 0.0

        # Обновление позиции и ориентации
        self.yaw += omega * dt
        self.yaw = normalize_angle(self.yaw)
        self.x += v_vehicle_ms * dt * np.cos(self.yaw)
        self.y += v_vehicle_ms * dt * np.sin(self.yaw)

        return self.x, self.y, self.yaw
