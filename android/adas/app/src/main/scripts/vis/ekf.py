#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extended Kalman Filter для локализации транспортного средства (Vehicle Localization)

Состояние: [x, y, yaw, v, yaw_rate] (5D)

Prediction (100Hz):
  - Bicycle model с использованием wheel speed и steering angle
  - IMU yaw_rate для улучшения динамики

Update GPS (1Hz):
  - Коррекция позиции (x, y)
  - Опционально: heading

Update IMU (200Hz):
  - Коррекция yaw_rate

Адаптивный Kalman Gain обеспечивает:
  - Быструю конвергенцию в начале (gain → 0.9)
  - Малую коррекцию при стабильном состоянии (gain → 0.05)
  - Автоматическое отклонение аномалий GPS
"""

import numpy as np
from typing import Tuple, Optional


class VehicleEKF:
    """
    Extended Kalman Filter для 5D состояния транспортного средства.

    State vector: x = [x, y, yaw, v, yaw_rate]^T

    x:        позиция X в локальных координатах (м)
    y:        позиция Y в локальных координатах (м)
    yaw:      ориентация (рад)
    v:        линейная скорость (м/с)
    yaw_rate: угловая скорость (рад/с)
    """

    def __init__(
        self,
        initial_x: float = 0.0,
        initial_y: float = 0.0,
        initial_yaw: float = 0.0,
        initial_v: float = 0.0,
        initial_yaw_rate: float = 0.0,
        wheelbase: float = 2.636,
        # Ковариация процесса (Process noise)
        process_noise_pos: float = 0.1,  # м (неопределенность позиции)
        process_noise_yaw: float = 0.01,  # рад (неопределенность ориентации)
        process_noise_v: float = 0.5,  # м/с (неопределенность скорости)
        process_noise_yaw_rate: float = 0.05,  # рад/с (неопределенность yaw_rate)
        # Ковариация измерений (Measurement noise)
        gps_noise_pos: float = 5.0,  # м (GPS точность)
        imu_noise_yaw_rate: float = 0.02,  # рад/с (IMU точность)
        # Начальная неопределенность
        initial_pos_uncertainty: float = 10.0,  # м
        initial_yaw_uncertainty: float = 0.5,  # рад (~30°)
        initial_v_uncertainty: float = 2.0,  # м/с
        initial_yaw_rate_uncertainty: float = 0.1,  # рад/с
    ):
        """
        Инициализация Extended Kalman Filter.

        Args:
            initial_x, initial_y, initial_yaw, initial_v, initial_yaw_rate: начальное состояние
            wheelbase: колесная база автомобиля (м)
            process_noise_*: шум процесса (model uncertainty)
            gps_noise_pos: шум GPS измерений
            imu_noise_yaw_rate: шум IMU измерений
            initial_*_uncertainty: начальная неопределенность состояния
        """
        self.wheelbase = wheelbase

        # Состояние: [x, y, yaw, v, yaw_rate]^T
        self.x = np.array([initial_x, initial_y, initial_yaw, initial_v, initial_yaw_rate])

        # Ковариационная матрица состояния P (5x5)
        # Начальная неопределенность: большая → EKF быстро доверяет измерениям
        self.P = np.diag(
            [
                initial_pos_uncertainty**2,  # var(x)
                initial_pos_uncertainty**2,  # var(y)
                initial_yaw_uncertainty**2,  # var(yaw)
                initial_v_uncertainty**2,  # var(v)
                initial_yaw_rate_uncertainty**2,  # var(yaw_rate)
            ]
        )

        # Матрица шума процесса Q (5x5)
        self.Q = np.diag(
            [
                process_noise_pos**2,
                process_noise_pos**2,
                process_noise_yaw**2,
                process_noise_v**2,
                process_noise_yaw_rate**2,
            ]
        )

        # Матрица шума измерений для GPS R_gps (2x2 для x, y)
        self.R_gps = np.diag([gps_noise_pos**2, gps_noise_pos**2])

        # Матрица шума измерений для IMU R_imu (1x1 для yaw_rate)
        self.R_imu = np.array([[imu_noise_yaw_rate**2]])

        # Статистика
        self.prediction_count = 0
        self.gps_update_count = 0
        self.gps_rejected_count = 0
        self.imu_update_count = 0

    def get_state(self) -> np.ndarray:
        """Возвращает текущее состояние [x, y, yaw, v, yaw_rate]."""
        return self.x.copy()

    def get_position(self) -> Tuple[float, float]:
        """Возвращает текущую позицию (x, y)."""
        return self.x[0], self.x[1]

    def get_yaw(self) -> float:
        """Возвращает текущую ориентацию yaw (рад)."""
        return self.x[2]

    def get_uncertainty(self) -> np.ndarray:
        """Возвращает диагональ ковариационной матрицы (стандартные отклонения)."""
        return np.sqrt(np.diag(self.P))

    @staticmethod
    def normalize_angle(angle: float) -> float:
        """Нормализует угол в диапазон [-π, π]."""
        return np.arctan2(np.sin(angle), np.cos(angle))

    def predict(self, v_measured: float, steering_angle: float, dt: float):
        """
        Prediction step: прогноз состояния на основе bicycle model.

        ВАЖНО: В prediction используем ТОЛЬКО одометрию (steering angle).
        IMU yaw_rate используется только в update_imu() для коррекции.
        Это предотвращает "information incest" (двойное использование данных).

        Args:
            v_measured: измеренная скорость из одометрии (м/с)
            steering_angle: угол руля (рад)
            dt: временной шаг (с)
        """
        # Текущее состояние
        x, y, yaw, v, yaw_rate = self.x

        # ===== MOTION MODEL (Bicycle Model) =====
        # ========================================

        # Yaw rate из модели велосипеда (bicycle model)
        # ТОЛЬКО из одометрии (steering angle)
        if abs(steering_angle) > 0.001 and abs(v_measured) > 0.01:
            yaw_rate_pred = v_measured * np.tan(steering_angle) / self.wheelbase
        else:
            yaw_rate_pred = 0.0

        # Прогноз состояния (nonlinear motion model)
        x_pred = x + v * np.cos(yaw) * dt
        y_pred = y + v * np.sin(yaw) * dt
        yaw_pred = self.normalize_angle(yaw + yaw_rate_pred * dt)
        v_pred = v_measured  # Скорость измеряется напрямую
        yaw_rate_state = yaw_rate_pred  # Предсказание yaw_rate из одометрии

        # Обновление состояния
        self.x = np.array([x_pred, y_pred, yaw_pred, v_pred, yaw_rate_state])

        # ===== JACOBIAN матрица F (linearization) =====
        # ==============================================
        # F = ∂f/∂x - производная motion model по состоянию

        F = np.eye(5)
        F[0, 2] = -v * np.sin(yaw) * dt  # ∂x/∂yaw
        F[0, 3] = np.cos(yaw) * dt  # ∂x/∂v
        F[1, 2] = v * np.cos(yaw) * dt  # ∂y/∂yaw
        F[1, 3] = np.sin(yaw) * dt  # ∂y/∂v
        F[2, 4] = dt  # ∂yaw/∂yaw_rate

        # Прогноз ковариации: P = F*P*F^T + Q
        self.P = F @ self.P @ F.T + self.Q

        self.prediction_count += 1

    def update_gps(self, gps_x: float, gps_y: float, max_innovation: float = 50.0) -> bool:
        """
        Update step: коррекция состояния по GPS измерениям.

        Args:
            gps_x: GPS измерение X (м)
            gps_y: GPS измерение Y (м)
            max_innovation: максимальное innovation для outlier rejection (м)

        Returns:
            True если обновление применено, False если отклонено (outlier)
        """
        # Измерение: z = [x_gps, y_gps]^T
        z = np.array([gps_x, gps_y])

        # Ожидаемое измерение: h(x) = [x, y]^T
        h = np.array([self.x[0], self.x[1]])

        # Innovation (невязка): y = z - h(x)
        innovation = z - h

        # ===== OUTLIER REJECTION =====
        # =============================
        # Проверяем, не слишком ли большая невязка (GPS jump, multipath)
        innovation_magnitude = np.linalg.norm(innovation)
        if innovation_magnitude > max_innovation:
            self.gps_rejected_count += 1
            return False  # Отклоняем измерение

        # Матрица измерений H (2x5): связь измерений с состоянием
        # z = H*x, где измеряем только [x, y]
        H = np.zeros((2, 5))
        H[0, 0] = 1.0  # измеряем x
        H[1, 1] = 1.0  # измеряем y

        # Innovation covariance: S = H*P*H^T + R
        S = H @ self.P @ H.T + self.R_gps

        # Kalman Gain: K = P*H^T*S^-1
        K = self.P @ H.T @ np.linalg.inv(S)

        # Обновление состояния: x = x + K*innovation
        self.x = self.x + K @ innovation

        # Нормализация угла
        self.x[2] = self.normalize_angle(self.x[2])

        # Обновление ковариации: P = (I - K*H)*P
        # Более численно стабильная форма Joseph:
        I_KH = np.eye(5) - K @ H
        self.P = I_KH @ self.P @ I_KH.T + K @ self.R_gps @ K.T

        self.gps_update_count += 1
        return True

    def update_imu(self, yaw_rate_imu: float):
        """
        Update step: коррекция yaw_rate по IMU измерениям.

        Args:
            yaw_rate_imu: измеренная угловая скорость из IMU (рад/с)
        """
        # Измерение: z = yaw_rate_imu
        z = np.array([yaw_rate_imu])

        # Ожидаемое измерение: h(x) = yaw_rate
        h = np.array([self.x[4]])

        # Innovation: y = z - h(x)
        innovation = z - h

        # Матрица измерений H (1x5): измеряем только yaw_rate
        H = np.zeros((1, 5))
        H[0, 4] = 1.0  # измеряем yaw_rate

        # Innovation covariance: S = H*P*H^T + R
        S = H @ self.P @ H.T + self.R_imu

        # Kalman Gain: K = P*H^T*S^-1
        K = self.P @ H.T @ np.linalg.inv(S)

        # Обновление состояния: x = x + K*innovation
        self.x = self.x + K @ innovation

        # Нормализация угла
        self.x[2] = self.normalize_angle(self.x[2])

        # Обновление ковариации: P = (I - K*H)*P
        I_KH = np.eye(5) - K @ H
        self.P = I_KH @ self.P @ I_KH.T + K @ self.R_imu @ K.T

        self.imu_update_count += 1

    def get_kalman_gain_magnitude(self) -> float:
        """
        Возвращает "эффективный" Kalman gain для GPS (для отладки).
        Показывает, насколько сильно EKF доверяет GPS vs предсказанию.

        Returns:
            Значение 0-1, где 1 = полное доверие GPS, 0 = игнорирование GPS
        """
        # Вычисляем Kalman gain для GPS (без применения update)
        H = np.zeros((2, 5))
        H[0, 0] = 1.0
        H[1, 1] = 1.0
        S = H @ self.P @ H.T + self.R_gps
        K = self.P @ H.T @ np.linalg.inv(S)

        # Берем среднее по диагонали первых двух элементов (x, y)
        return (K[0, 0] + K[1, 1]) / 2.0

    def print_statistics(self):
        """Печатает статистику работы EKF."""
        print("\n" + "=" * 60)
        print("📊 EKF СТАТИСТИКА:")
        print("=" * 60)
        print(f"  Predictions:      {self.prediction_count}")
        print(f"  GPS updates:      {self.gps_update_count}")
        print(f"  GPS rejected:     {self.gps_rejected_count}")
        print(f"  IMU updates:      {self.imu_update_count}")

        uncertainties = self.get_uncertainty()
        print(f"\n  Финальная неопределенность (σ):")
        print(f"    x:        {uncertainties[0]:.2f} м")
        print(f"    y:        {uncertainties[1]:.2f} м")
        print(f"    yaw:      {np.degrees(uncertainties[2]):.2f}°")
        print(f"    v:        {uncertainties[3]:.2f} м/с")
        print(f"    yaw_rate: {np.degrees(uncertainties[4]):.2f}°/с")

        kalman_gain = self.get_kalman_gain_magnitude()
        print(f"\n  Текущий Kalman Gain: {kalman_gain:.3f}")
        print(f"    (0 = доверяем предсказанию, 1 = доверяем GPS)")
        print("=" * 60)
