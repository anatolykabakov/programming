#!/usr/bin/env python3
"""
Интерактивный визуализатор логов ADAS системы.

Отображает:
- Траекторию движения (2D plot)
- Синхронизированное изображение с камеры
- Данные сенсоров в реальном времени
- Временную шкалу с навигацией

Интерфейс:
┌─────────────────────────────────────────────┐
│  Trajectory Plot          │  Camera View    │
│                          │                  │
│  [2D trajectory]         │  [Image frame]   │
│                          │                  │
├─────────────────────────────────────────────┤
│  Timeline: [════●════════] 00:01:23        │
├─────────────────────────────────────────────┤
│  IMU: ax=1.2 ay=0.5 gz=0.1                │
│  GPS: lat=55.7558 lon=37.6173             │
│  Odom: vl=5.2 vr=5.3 gear=D               │
└─────────────────────────────────────────────┘

Использование:
    python3 interactive_visualizer.py --input /path/to/logs
    python3 interactive_visualizer.py --input logs.zip
"""

import numpy as np
import matplotlib

matplotlib.use("TkAgg")  # Интерактивный бэкенд
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import argparse
import os
from pathlib import Path
import cv2

# Импорты модулей
from log_parser import LogParser
from gps_utils import gps_to_local_coords, calculate_initial_heading_from_gps
from imu_utils import process_imu_for_odometry
from trajectory_calculators import (
    calculate_trajectory,
    calculate_trajectory_imu,
    calculate_trajectory_fusion,
    calculate_trajectory_ekf,
)


class InteractiveVisualizer:
    """Интерактивный визуализатор данных ADAS"""

    def __init__(self, master, log_path=None):
        self.master = master
        self.master.title("ADAS Interactive Visualizer")
        self.master.geometry("1400x900")

        # Данные
        self.log_parser = None
        self.trajectories = {}
        self.images = []
        self.image_timestamps = []  # Кэш timestamps изображений
        self.timestamps = []
        self.current_index = 0

        # Данные сенсоров
        self.imu_data = None
        self.gps_data = None
        self.wheel_data = None
        self.steering_data = None
        self.gear_data = None

        # Создаем UI
        self.create_ui()

        # Если путь указан - загружаем
        if log_path:
            self.load_logs(log_path)

    def create_ui(self):
        """Создание интерфейса"""

        # ========== Верхняя панель - Кнопки управления ==========
        control_frame = ttk.Frame(self.master)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        ttk.Button(control_frame, text="📁 Открыть логи", command=self.open_logs).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(control_frame, text="▶️ Play", command=self.play).pack(side=tk.LEFT, padx=5)
        ttk.Button(control_frame, text="⏸️ Pause", command=self.pause).pack(side=tk.LEFT, padx=5)
        ttk.Button(control_frame, text="⏮️ Restart", command=self.restart).pack(
            side=tk.LEFT, padx=5
        )

        # Speed control
        ttk.Label(control_frame, text="Скорость:").pack(side=tk.LEFT, padx=(20, 5))
        self.speed_var = tk.DoubleVar(value=1.0)
        speed_slider = ttk.Scale(
            control_frame,
            from_=0.1,
            to=5.0,
            orient=tk.HORIZONTAL,
            variable=self.speed_var,
            length=100,
        )
        speed_slider.pack(side=tk.LEFT, padx=5)
        self.speed_label = ttk.Label(control_frame, text="1.0x")
        self.speed_label.pack(side=tk.LEFT, padx=5)
        self.speed_var.trace_add("write", self.on_speed_change)

        self.status_label = ttk.Label(control_frame, text="Готов", foreground="green")
        self.status_label.pack(side=tk.RIGHT, padx=10)

        # ========== Основная область - Графики и Камера ==========
        main_frame = ttk.Frame(self.master)
        main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Левая половина - Траектория
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Правая половина - Камера
        right_frame = ttk.Frame(main_frame)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # --- Траектория ---
        self.fig_trajectory = Figure(figsize=(6, 6), dpi=100)
        self.ax_trajectory = self.fig_trajectory.add_subplot(111)
        self.ax_trajectory.set_title("Vehicle Trajectory")
        self.ax_trajectory.set_xlabel("X (meters)")
        self.ax_trajectory.set_ylabel("Y (meters)")
        self.ax_trajectory.grid(True, alpha=0.3)
        self.ax_trajectory.set_aspect("equal")

        self.canvas_trajectory = FigureCanvasTkAgg(self.fig_trajectory, left_frame)
        self.canvas_trajectory.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # Маркер текущей позиции
        (self.current_pos_marker,) = self.ax_trajectory.plot(
            [], [], "ro", markersize=10, label="Current Position"
        )

        # --- Камера ---
        ttk.Label(right_frame, text="Camera View", font=("Arial", 14, "bold")).pack(pady=5)

        self.camera_canvas = tk.Canvas(right_frame, bg="black", width=640, height=480)
        self.camera_canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Placeholder изображение
        self.camera_image_id = None

        # ========== Timeline - Временная шкала ==========
        timeline_frame = ttk.LabelFrame(self.master, text="Timeline", padding=10)
        timeline_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        # Слайдер
        slider_frame = ttk.Frame(timeline_frame)
        slider_frame.pack(fill=tk.X)

        self.time_var = tk.IntVar(value=0)
        self.timeline_slider = ttk.Scale(
            slider_frame,
            from_=0,
            to=100,
            orient=tk.HORIZONTAL,
            variable=self.time_var,
            command=self.on_timeline_change,
        )
        self.timeline_slider.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

        self.time_label = ttk.Label(slider_frame, text="00:00:00 / 00:00:00", font=("Courier", 10))
        self.time_label.pack(side=tk.RIGHT, padx=10)

        # ========== Sensor Data - Данные сенсоров ==========
        sensor_frame = ttk.LabelFrame(self.master, text="Sensor Data", padding=10)
        sensor_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)

        # Создаем три колонки для сенсоров
        imu_frame = ttk.Frame(sensor_frame)
        imu_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)

        gps_frame = ttk.Frame(sensor_frame)
        gps_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)

        odom_frame = ttk.Frame(sensor_frame)
        odom_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=10)

        # IMU данные
        ttk.Label(imu_frame, text="📊 IMU", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.imu_text = tk.Text(imu_frame, height=4, width=35, font=("Courier", 9))
        self.imu_text.pack(fill=tk.BOTH, expand=True)
        self.imu_text.config(state=tk.DISABLED)

        # GPS данные
        ttk.Label(gps_frame, text="🌍 GPS", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.gps_text = tk.Text(gps_frame, height=4, width=35, font=("Courier", 9))
        self.gps_text.pack(fill=tk.BOTH, expand=True)
        self.gps_text.config(state=tk.DISABLED)

        # Одометрия
        ttk.Label(odom_frame, text="🚗 Odometry", font=("Arial", 11, "bold")).pack(anchor=tk.W)
        self.odom_text = tk.Text(odom_frame, height=4, width=35, font=("Courier", 9))
        self.odom_text.pack(fill=tk.BOTH, expand=True)
        self.odom_text.config(state=tk.DISABLED)

        # Состояние воспроизведения
        self.playing = False
        self.play_speed = 1.0  # Скорость воспроизведения

    def open_logs(self):
        """Открыть диалог выбора логов"""
        path = filedialog.askdirectory(title="Выберите директорию с логами")
        if path:
            self.load_logs(path)

    def load_logs(self, log_path):
        """Загрузка и парсинг логов"""
        try:
            self.status_label.config(text="Загрузка логов...", foreground="orange")
            self.master.update()

            print(f"Загрузка логов из: {log_path}")
            self.log_parser = LogParser(log_path)
            self.log_parser.parse_all()

            # Получить данные
            self.imu_data = self.log_parser.get_imu()
            self.gps_data = self.log_parser.get_gps()
            self.wheel_data = self.log_parser.get_wheel_speed()
            self.steering_data = self.log_parser.get_steering()
            self.gear_data = self.log_parser.get_gear()
            self.images = self.log_parser.get_images()

            # Рассчитать траектории
            self.calculate_trajectories()

            # Построить траектории
            self.plot_trajectories()

            # Настроить timeline
            if self.wheel_data and len(self.wheel_data) > 0:
                # Конвертировать в numpy array если это list
                if isinstance(self.wheel_data, list):
                    self.wheel_data = np.array(self.wheel_data)
                self.timestamps = self.wheel_data[:, 0]
                self.timeline_slider.config(to=len(self.timestamps) - 1)
                self.time_var.set(0)

            # Кэшировать timestamps изображений для быстрого поиска
            self.cache_image_timestamps()

            # Обновить отображение
            self.update_display(0)

            summary = self.log_parser.get_summary()
            self.status_label.config(
                text=f"✅ Загружено: {summary['imu_records']} IMU, {summary['gps_records']} GPS, {summary['camera_images']} изображений",
                foreground="green",
            )

        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось загрузить логи:\n{e}")
            self.status_label.config(text=f"❌ Ошибка: {e}", foreground="red")
            import traceback

            traceback.print_exc()

    def calculate_trajectories(self):
        """Расчет всех траекторий"""
        print("\n" + "=" * 60)
        print("Расчет траекторий...")
        print("=" * 60)

        if not self.wheel_data or len(self.wheel_data) == 0:
            print("Нет данных одометрии")
            return

        # Начальная ориентация по GPS
        initial_yaw = 0.0
        if self.gps_data is not None and len(self.gps_data) >= 2:
            initial_yaw = calculate_initial_heading_from_gps(self.gps_data, n_points=5)
            print(f"Начальная ориентация по GPS: {np.degrees(initial_yaw):.1f}°")

        # GPS траектория
        if self.gps_data is not None and len(self.gps_data) > 0:
            x_gps, y_gps = gps_to_local_coords(self.gps_data, origin_idx=0)
            self.trajectories["GPS"] = (x_gps, y_gps)
            print(f"GPS траектория: {len(x_gps)} точек")

        # Конвертировать данные в numpy arrays
        wheel_array = (
            np.array(self.wheel_data) if isinstance(self.wheel_data, list) else self.wheel_data
        )
        steering_array = (
            np.array(self.steering_data)
            if isinstance(self.steering_data, list) and self.steering_data
            else self.steering_data
        )
        gear_array = (
            np.array(self.gear_data)
            if isinstance(self.gear_data, list) and self.gear_data
            else self.gear_data
        )

        # Одометрия
        x_odom, y_odom = calculate_trajectory(
            wheel_array, steering_array, gear_array, wheelbase=2.636, initial_yaw=initial_yaw
        )
        self.trajectories["Odometry"] = (x_odom, y_odom)
        print(f"Одометрия: {len(x_odom)} точек")

        # IMU траектории
        imu_array = np.array(self.imu_data) if isinstance(self.imu_data, list) else self.imu_data

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

                # IMU траектория
                x_imu, y_imu = calculate_trajectory_imu(
                    wheel_array,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    initial_yaw=initial_yaw,
                )
                self.trajectories["IMU"] = (x_imu, y_imu)
                print(f"IMU траектория: {len(x_imu)} точек")
                # EKF траектория
                x_ekf, y_ekf, _ = calculate_trajectory_ekf(
                    wheel_array,
                    steering_array,
                    gear_array,
                    imu_processed["yaw_rate"],
                    imu_processed["imu_calibrated"][:, 0],
                    self.gps_data,
                    wheelbase=2.636,
                    initial_yaw=initial_yaw,
                )
                self.trajectories["EKF"] = (x_ekf, y_ekf)
                print(f"EKF траектория: {len(x_ekf)} точек")

            except Exception as e:
                print(f"⚠️ Ошибка обработки IMU: {e}")
                import traceback

                traceback.print_exc()

        print("=" * 60 + "\n")

    def plot_trajectories(self):
        """Построение траекторий на графике"""
        self.ax_trajectory.clear()
        self.ax_trajectory.set_title("Vehicle Trajectory", fontsize=14, fontweight="bold")
        self.ax_trajectory.set_xlabel("X (meters)", fontsize=11)
        self.ax_trajectory.set_ylabel("Y (meters)", fontsize=11)
        self.ax_trajectory.grid(True, alpha=0.3, linestyle="--")
        self.ax_trajectory.set_aspect("equal")

        # Цвета и стили для разных траекторий
        styles = {
            "Odometry": {"color": "blue", "linestyle": "-", "linewidth": 2, "alpha": 0.7},
            "GPS": {"color": "red", "linestyle": "--", "linewidth": 2, "alpha": 0.7},
            "IMU": {"color": "green", "linestyle": "-.", "linewidth": 1.5, "alpha": 0.6},
            "EKF": {"color": "orange", "linestyle": "-", "linewidth": 2.5, "alpha": 0.9},
        }

        # Отрисовка всех траекторий
        for name, (x, y) in self.trajectories.items():
            if x is not None and len(x) > 0:
                style = styles.get(name, {})
                self.ax_trajectory.plot(x, y, label=name, **style)

        # Маркер текущей позиции
        (self.current_pos_marker,) = self.ax_trajectory.plot(
            [],
            [],
            "ro",
            markersize=12,
            label="Current",
            zorder=10,
            markeredgecolor="white",
            markeredgewidth=2,
        )

        # Легенда
        self.ax_trajectory.legend(loc="upper right", fontsize=10, framealpha=0.9)

        self.canvas_trajectory.draw()

    def update_display(self, index):
        """Обновление всех элементов интерфейса для текущего индекса"""
        if not self.trajectories:
            return

        self.current_index = min(index, len(self.timestamps) - 1 if len(self.timestamps) > 0 else 0)

        # Обновить текущую позицию на траектории
        if "EKF" in self.trajectories:
            x, y = self.trajectories["EKF"]
        elif "Odometry" in self.trajectories:
            x, y = self.trajectories["Odometry"]
        else:
            return

        if self.current_index < len(x):
            self.current_pos_marker.set_data([x[self.current_index]], [y[self.current_index]])
            self.canvas_trajectory.draw_idle()

        # Обновить изображение с камеры
        self.update_camera_view(self.current_index)

        # Обновить данные сенсоров
        self.update_sensor_data(self.current_index)

        # Обновить время
        self.update_time_label(self.current_index)

    def update_camera_view(self, index):
        """Обновление изображения с камеры"""
        if not self.images or len(self.images) == 0:
            return

        # Найти ближайшее изображение по реальному времени
        if len(self.timestamps) > 0 and index < len(self.timestamps):
            current_time = self.timestamps[index]

            # Извлечь timestamp из имени файла изображения и найти ближайшее
            img_index = self.find_closest_image_by_time(current_time)

            if img_index is not None and img_index < len(self.images):
                img_path = self.images[img_index]
                img = self.log_parser.read_camera_image(img_path)

                if img is not None:
                    # Изменить размер для отображения
                    canvas_width = self.camera_canvas.winfo_width()
                    canvas_height = self.camera_canvas.winfo_height()

                    if canvas_width > 1 and canvas_height > 1:
                        # Сохранить aspect ratio
                        img_height, img_width = img.shape[:2]
                        scale = min(canvas_width / img_width, canvas_height / img_height)
                        new_width = int(img_width * scale)
                        new_height = int(img_height * scale)

                        img_resized = cv2.resize(img, (new_width, new_height))

                        # Конвертация BGR -> RGB для Tkinter
                        img_rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB)

                        # Создать PhotoImage
                        from PIL import Image, ImageTk

                        pil_image = Image.fromarray(img_rgb)
                        photo = ImageTk.PhotoImage(image=pil_image)

                        # Обновить canvas
                        self.camera_canvas.delete("all")
                        x_offset = (canvas_width - new_width) // 2
                        y_offset = (canvas_height - new_height) // 2
                        self.camera_canvas.create_image(
                            x_offset, y_offset, image=photo, anchor=tk.NW
                        )

                        # Сохранить ссылку (иначе garbage collector удалит)
                        self.camera_canvas.image = photo

    def update_sensor_data(self, index):
        """Обновление текстовых данных сенсоров"""

        # IMU данные
        imu_array = np.array(self.imu_data) if isinstance(self.imu_data, list) else self.imu_data
        if imu_array is not None and index < len(imu_array):
            imu = imu_array[index]
            imu_text = f"Accel: ax={imu[1]:6.2f} ay={imu[2]:6.2f} az={imu[3]:6.2f} m/s²\n"
            imu_text += f"Gyro:  gx={imu[4]:6.3f} gy={imu[5]:6.3f} gz={imu[6]:6.3f} rad/s\n"
            imu_text += f"Mag:   mx={imu[7]:6.1f} my={imu[8]:6.1f} mz={imu[9]:6.1f} μT"
            self.update_text_widget(self.imu_text, imu_text)

        # GPS данные
        gps_array = np.array(self.gps_data) if isinstance(self.gps_data, list) else self.gps_data
        if gps_array is not None and index < len(gps_array):
            gps = gps_array[index]
            gps_text = f"Lat:     {gps[1]:12.8f}°\n"
            gps_text += f"Lon:     {gps[2]:12.8f}°\n"
            gps_text += f"Alt:     {gps[3]:8.2f} m\n"
            if len(gps) > 4:
                gps_text += f"Speed:   {gps[4]:6.2f} m/s"
            self.update_text_widget(self.gps_text, gps_text)

        # Одометрия
        wheel_array = (
            np.array(self.wheel_data) if isinstance(self.wheel_data, list) else self.wheel_data
        )
        steering_array = (
            np.array(self.steering_data)
            if isinstance(self.steering_data, list)
            else self.steering_data
        )
        gear_array = (
            np.array(self.gear_data) if isinstance(self.gear_data, list) else self.gear_data
        )

        if wheel_array is not None and index < len(wheel_array):
            wheel = wheel_array[index]
            odom_text = f"Wheels: FL={wheel[1]:5.2f} FR={wheel[2]:5.2f} m/s\n"
            odom_text += f"        RL={wheel[3]:5.2f} RR={wheel[4]:5.2f} m/s\n"

            if steering_array is not None and index < len(steering_array):
                steering = steering_array[index]
                odom_text += f"Steer:  {steering[1]:6.2f} deg\n"

            if gear_array is not None and index < len(gear_array):
                gear = gear_array[index]
                # gear[1] может быть строкой ('PARK', 'DRIVE') или числом
                gear_value = str(gear[1]).upper()

                # Мапинг передач
                if gear_value in ["PARK", "P", "0"]:
                    gear_str = "P"
                elif gear_value in ["REVERSE", "R", "1"]:
                    gear_str = "R"
                elif gear_value in ["NEUTRAL", "N", "2"]:
                    gear_str = "N"
                elif gear_value in ["DRIVE", "D", "3", "4", "5", "6"]:
                    gear_str = "D"
                else:
                    gear_str = "?"

                odom_text += f"Gear:   {gear_str}"

            self.update_text_widget(self.odom_text, odom_text)

    def update_text_widget(self, widget, text):
        """Обновить текстовый виджет"""
        widget.config(state=tk.NORMAL)
        widget.delete(1.0, tk.END)
        widget.insert(1.0, text)
        widget.config(state=tk.DISABLED)

    def update_time_label(self, index):
        """Обновить метку времени"""
        if len(self.timestamps) == 0:
            return

        current_time = self.timestamps[index]
        total_time = self.timestamps[-1]
        start_time = self.timestamps[0]

        elapsed = current_time - start_time
        duration = total_time - start_time

        # Форматирование времени
        current_str = self.format_time(elapsed)
        total_str = self.format_time(duration)

        self.time_label.config(text=f"{current_str} / {total_str}")

    @staticmethod
    def format_time(microseconds):
        """Форматирование микросекунд в ЧЧ:ММ:СС"""
        seconds = microseconds / 1_000_000
        hours = int(seconds // 3600)
        minutes = int((seconds % 3600) // 60)
        secs = int(seconds % 60)
        return f"{hours:02d}:{minutes:02d}:{secs:02d}"

    def on_timeline_change(self, value):
        """Обработчик изменения слайдера"""
        try:
            index = int(float(value))
            self.update_display(index)
        except Exception as e:
            print(f"Ошибка при изменении timeline: {e}")

    def on_speed_change(self, *args):
        """Обработчик изменения скорости"""
        speed = self.speed_var.get()
        self.play_speed = speed
        self.speed_label.config(text=f"{speed:.1f}x")

    def cache_image_timestamps(self):
        """Кэшировать timestamps изображений для быстрого поиска"""
        print("Кэширование timestamps изображений...")
        self.image_timestamps = []

        for img_path in self.images:
            try:
                # Извлечь timestamp из имени файла (формат: camera/1759950098876.png)
                filename = Path(img_path).stem
                img_timestamp = int(filename)
                self.image_timestamps.append(img_timestamp)
            except (ValueError, AttributeError):
                # Если не можем извлечь, используем -1
                self.image_timestamps.append(-1)

        self.image_timestamps = np.array(self.image_timestamps)
        print(f"Закэшировано {len(self.image_timestamps)} timestamps изображений")

    def find_closest_image_by_time(self, target_time):
        """Найти изображение с ближайшим timestamp используя бинарный поиск"""
        if len(self.image_timestamps) == 0:
            return None

        # Удалить невалидные timestamps
        valid_mask = self.image_timestamps > 0
        if not valid_mask.any():
            return 0

        valid_timestamps = self.image_timestamps[valid_mask]
        valid_indices = np.where(valid_mask)[0]

        # Найти ближайший timestamp используя numpy searchsorted
        idx = np.searchsorted(valid_timestamps, target_time)

        # Проверить граничные случаи
        if idx == 0:
            return valid_indices[0]
        elif idx >= len(valid_timestamps):
            return valid_indices[-1]
        else:
            # Выбрать ближайший из двух соседних
            before_idx = idx - 1
            after_idx = idx

            diff_before = abs(valid_timestamps[before_idx] - target_time)
            diff_after = abs(valid_timestamps[after_idx] - target_time)

            if diff_before < diff_after:
                return valid_indices[before_idx]
            else:
                return valid_indices[after_idx]

    def play(self):
        """Начать воспроизведение"""
        if not self.trajectories:
            messagebox.showwarning("Предупреждение", "Сначала загрузите логи")
            return

        self.playing = True
        self.animate()

    def pause(self):
        """Пауза"""
        self.playing = False

    def restart(self):
        """Перезапуск с начала"""
        self.time_var.set(0)
        self.update_display(0)
        self.playing = False

    def animate(self):
        """Анимация воспроизведения"""
        if not self.playing:
            return

        current = self.time_var.get()
        max_val = int(self.timeline_slider.cget("to"))

        if current < max_val:
            # Следующий кадр (пропускаем кадры для ускорения)
            step = max(1, int(50 * self.play_speed))  # Пропускаем кадры для быстрого просмотра
            next_index = min(current + step, max_val)
            self.time_var.set(next_index)
            self.update_display(next_index)

            # Планируем следующее обновление
            delay = 50  # 50ms = 20 FPS (плавное воспроизведение)
            self.master.after(delay, self.animate)
        else:
            # Достигли конца
            self.playing = False
            self.status_label.config(text="✅ Воспроизведение завершено", foreground="blue")


def main():
    parser = argparse.ArgumentParser(description="Интерактивный визуализатор ADAS логов")
    parser.add_argument("--input", "-i", help="Путь к директории с логами или ZIP архиву")

    args = parser.parse_args()

    # Создать главное окно
    root = tk.Tk()
    app = InteractiveVisualizer(root, log_path=args.input)

    print("\n" + "=" * 60)
    print("🚀 Интерактивный визуализатор запущен")
    print("=" * 60)
    print("Управление:")
    print("  - 📁 Открыть логи: выбрать директорию с логами")
    print("  - ▶️  Play: воспроизвести движение")
    print("  - ⏸️  Pause: остановить воспроизведение")
    print("  - ⏮️  Restart: начать сначала")
    print("  - Слайдер: перемотка на любой момент времени")
    print("=" * 60 + "\n")

    # Запустить главный цикл
    root.mainloop()

    return 0


if __name__ == "__main__":
    exit(main())
