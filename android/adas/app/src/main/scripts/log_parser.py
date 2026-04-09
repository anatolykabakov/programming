#!/usr/bin/env python3
"""
Парсер логов ADAS системы.
Поддерживает чтение из директории или ZIP архива.
"""
import numpy as np
import zipfile
import cv2
from pathlib import Path
from typing import Dict, List, Tuple, Optional


class LogParser:
    """
    Парсер логов ADAS системы.
    Поддерживает чтение из директории или ZIP архива.

    Формат логов:
    - imu.txt: timestamp_ms, ax, ay, az, gx, gy, gz, mx, my, mz
    - gps.txt: timestamp_ms, lat, lon, alt, speed
    - can.txt: timestamp, 0xADDRESS, busTime, src, data_size, hex_data
    - camera/: изображения и intrinsics.txt
    """

    def __init__(self, log_path: str):
        """
        Инициализация парсера.

        Args:
            log_path: Путь к директории с логами или ZIP архиву
        """
        self.log_path = Path(log_path)
        self.is_zip = self.log_path.suffix == ".zip"
        self.zip_file = None

        if self.is_zip:
            self.zip_file = zipfile.ZipFile(self.log_path, "r")
            # Получить имя директории внутри архива (обычно совпадает с именем архива)
            namelist = self.zip_file.namelist()
            self.base_dir = namelist[0].split("/")[0] if namelist else ""
        else:
            self.base_dir = self.log_path

        self.imu_data = None
        self.gps_data = None
        self.can_data = None
        self.odom_data = None  # Одометрия
        self.wheel_speed = []  # Скорости колес
        self.steering = []  # Углы поворота руля
        self.gear = []  # Передачи
        self.camera_images = []
        self.camera_intrinsics = None

    def __del__(self):
        """Закрыть ZIP файл при удалении объекта"""
        if self.zip_file:
            self.zip_file.close()

    def _read_file(self, filename: str) -> Optional[List[str]]:
        """
        Читает файл из директории или ZIP архива.
        Пытается найти файл с расширениями .txt и .log

        Args:
            filename: Имя файла относительно базовой директории (без расширения или с расширением)

        Returns:
            Список строк файла или None
        """
        # Попробовать разные расширения
        extensions = ["", ".txt", ".log"]
        base_name = filename.rsplit(".", 1)[0] if "." in filename else filename

        for ext in extensions:
            try_filename = base_name + ext if ext else filename

            try:
                if self.is_zip:
                    filepath = f"{self.base_dir}/{try_filename}"
                    with self.zip_file.open(filepath, "r") as f:
                        return [line.decode("utf-8").strip() for line in f.readlines()]
                else:
                    filepath = self.base_dir / try_filename
                    if filepath.exists():
                        with open(filepath, "r") as f:
                            return [line.strip() for line in f.readlines()]
            except:
                continue

        print(f"Файл не найден: {base_name}.txt/.log")
        return None

    def parse_imu(self) -> Optional[np.ndarray]:
        """
        Парсит imu.txt/imu.log файл.
        Поддерживает два формата:
        - Новый: запятая+пробел как разделитель, точка как десятичный разделитель
        - Старый: запятая как разделитель полей И как десятичный разделитель

        Returns:
            numpy array с shape (N, 10): [timestamp, ax, ay, az, gx, gy, gz, mx, my, mz]
        """
        lines = self._read_file("imu")
        if not lines:
            return None

        data = []
        for line in lines:
            if not line:
                continue
            try:
                # Разделить по запятой (в нашем формате используется запятая без пробела)
                parts = line.split(",")

                if len(parts) != 10:
                    continue

                # Все числа уже в правильном формате с точкой
                timestamp = int(float(parts[0].strip()))
                ax = float(parts[1].strip())
                ay = float(parts[2].strip())
                az = float(parts[3].strip())
                gx = float(parts[4].strip())
                gy = float(parts[5].strip())
                gz = float(parts[6].strip())
                mx = float(parts[7].strip())
                my = float(parts[8].strip())
                mz = float(parts[9].strip())

                data.append([timestamp, ax, ay, az, gx, gy, gz, mx, my, mz])
            except (ValueError, IndexError) as e:
                # print(f"Ошибка парсинга IMU строки: {line[:50]}... - {e}")
                continue

        self.imu_data = np.array(data) if data else None
        print(f"Загружено {len(data)} записей IMU")
        return self.imu_data

    def parse_gps(self) -> Optional[np.ndarray]:
        """
        Парсит gps.txt/gps.log файл.
        Поддерживает два формата:
        - Новый: запятая+пробел как разделитель, точка как десятичный разделитель
        - Старый: запятая как разделитель полей И как десятичный разделитель

        Returns:
            numpy array с shape (N, 5): [timestamp, lat, lon, alt, speed]
        """
        lines = self._read_file("gps")
        if not lines:
            return None

        data = []
        for line in lines:
            if not line:
                continue
            try:
                parts = line.split(",")

                if len(parts) != 5:
                    continue

                timestamp = int(float(parts[0].strip()))
                lat = float(parts[1].strip())
                lon = float(parts[2].strip())
                alt = float(parts[3].strip())
                speed = float(parts[4].strip())

                data.append([timestamp, lat, lon, alt, speed])
            except (ValueError, IndexError) as e:
                # print(f"Ошибка парсинга GPS строки: {line[:50]}... - {e}")
                continue

        self.gps_data = np.array(data) if data else None
        print(f"Загружено {len(data)} записей GPS")
        return self.gps_data

    def parse_can(self) -> Optional[List[Dict]]:
        """
        Парсит can.txt файл.

        Returns:
            Список словарей с CAN фреймами
        """
        lines = self._read_file("can_frames.txt")
        if not lines:
            return None

        data = []
        for line in lines:
            if not line:
                continue
            try:
                # Формат: timestamp,0xADDRESS,busTime,src,data_size,hex_data
                parts = line.split(",")
                if len(parts) < 6:
                    continue

                frame = {
                    "timestamp": int(parts[0]),
                    "address": parts[1],  # 0xXXX
                    "busTime": int(parts[2]),
                    "src": int(parts[3]),
                    "data_size": int(parts[4]),
                    "data": parts[5].strip() if len(parts) > 5 else "",
                }
                data.append(frame)
            except (ValueError, IndexError) as e:
                print(f"Ошибка парсинга CAN строки: {line} - {e}")
                continue

        self.can_data = data if data else None
        print(f"Загружено {len(data)} CAN фреймов")
        return self.can_data

    def list_camera_images(self) -> List[str]:
        """
        Получает список изображений из camera/ директории.

        Returns:
            Список путей к изображениям
        """
        try:
            if self.is_zip:
                # Найти все файлы в camera/ директории
                camera_prefix = f"{self.base_dir}/camera/"
                images = [
                    name
                    for name in self.zip_file.namelist()
                    if name.startswith(camera_prefix)
                    and (name.endswith(".jpg") or name.endswith(".png"))
                ]
                self.camera_images = sorted(images)
            else:
                camera_dir = self.base_dir / "camera"
                if camera_dir.exists():
                    images = list(camera_dir.glob("*.jpg")) + list(camera_dir.glob("*.png"))
                    self.camera_images = sorted([str(img) for img in images])
                else:
                    self.camera_images = []

            print(f"Найдено {len(self.camera_images)} изображений")
            return self.camera_images
        except Exception as e:
            print(f"Ошибка поиска изображений: {e}")
            return []

    def read_camera_image(self, image_path: str) -> Optional[np.ndarray]:
        """
        Читает изображение из camera/ директории.

        Args:
            image_path: Путь к изображению

        Returns:
            numpy array с изображением или None
        """
        try:
            if self.is_zip:
                with self.zip_file.open(image_path, "r") as f:
                    img_data = f.read()
                    img_array = np.frombuffer(img_data, np.uint8)
                    img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                    return img
            else:
                img = cv2.imread(image_path)
                return img
        except Exception as e:
            print(f"Ошибка чтения изображения {image_path}: {e}")
            return None

    def parse_odom(self) -> Tuple[List, List, List]:
        """
        Парсит odom.txt файл (одометрия из CAN).
        Поддерживает два формата:
        - Старый: WHEEL SPEED: timestamp,vr,vl,hr,hl
                  STEERING: timestamp,angle_abs,sign
                  GEAR: timestamp,gear_name,gear_value
        - Новый: timestamp, vr, vl, hr, hl, gear (простой CSV)

        Returns:
            Tuple из трех списков: (wheel_speed, steering, gear)
        """
        lines = self._read_file("odom")
        if not lines:
            return [], [], []

        wheel = []
        steering = []
        gear = []
        vehicle_speed = []

        for line in lines:
            if not line:
                continue

            try:
                # Старый формат с префиксами
                if line.startswith("WHEEL SPEED:"):
                    # Формат: WHEEL SPEED: timestamp,vr,vl,hr,hl
                    parts = line.split(":")[1].strip().split(",")
                    if len(parts) != 5:
                        continue

                    timestamp = int(float(parts[0]) / 1000)
                    vr = float(parts[1])  # км/ч
                    vl = float(parts[2])  # км/ч
                    hr = float(parts[3])  # км/ч
                    hl = float(parts[4])  # км/ч

                    wheel.append([timestamp, vr, vl, hr, hl])

                elif line.startswith("STEERING:"):
                    # Формат: STEERING: timestamp,angle_abs,sign
                    parts = line.split(":")[1].strip().split(",")
                    if len(parts) != 3:
                        continue

                    timestamp = int(float(parts[0]) / 1000)
                    angle_abs = float(parts[1])  # градусы
                    sign = float(parts[2])  # 0=влево, 1=вправо

                    steering.append([timestamp, angle_abs, sign])

                elif line.startswith("GEAR:"):
                    # Формат: GEAR: timestamp,gear_name,gear_value
                    parts = line.split(":")[1].strip().split(",")
                    if len(parts) != 3:
                        continue

                    timestamp = int(float(parts[0]) / 1000)
                    gear_name = parts[1]  # "PARK", "REVERSE", "DRIVE", etc.
                    gear_value = int(parts[2])  # 5, 6, 7, 8, etc.

                    gear.append([timestamp, gear_name, gear_value])

                elif line.startswith("VEHICLE SPEED:"):
                    # Формат: VEHICLE SPEED: timestamp,speed
                    parts = line.split(":")[1].strip().split(",")
                    if len(parts) != 2:
                        continue

                    timestamp = int(float(parts[0]) / 1000)
                    speed = float(parts[1])

                    vehicle_speed.append([timestamp, speed])

                # Новый формат (простой CSV): timestamp, vr, vl, hr, hl, gear
                elif not line.startswith("#"):  # игнорировать комментарии
                    parts = line.split(", ")
                    if len(parts) == 6:
                        timestamp = int(float(parts[0].replace(",", ".")))
                        vr = float(parts[1].replace(",", "."))
                        vl = float(parts[2].replace(",", "."))
                        hr = float(parts[3].replace(",", "."))
                        hl = float(parts[4].replace(",", "."))
                        gear_val = parts[5].strip()

                        wheel.append([timestamp, vr, vl, hr, hl])
                        # В новом формате gear может быть строкой или числом
                        try:
                            gear.append([timestamp, gear_val, int(gear_val)])
                        except:
                            gear.append([timestamp, gear_val, 0])

            except (ValueError, IndexError) as e:
                # print(f"Ошибка парсинга ODOM строки: {line[:50]}... - {e}")
                continue

        self.wheel_speed = wheel
        self.steering = steering
        self.gear = gear

        print(f"Загружено {len(wheel)} записей скоростей колес")
        print(f"Загружено {len(steering)} записей углов поворота руля")
        print(f"Загружено {len(gear)} записей передач")

        return wheel, steering, gear

    def parse_camera_intrinsics(self) -> Optional[Dict]:
        """
        Парсит camera/intrinsics.txt или camera_intrinsics.log файл.

        Returns:
            Словарь с параметрами камеры
        """
        # Попробовать разные пути
        lines = self._read_file("camera/intrinsics")
        if not lines:
            lines = self._read_file("camera_intrinsics")
        if not lines:
            return None

        intrinsics = {"raw_text": "\n".join(lines)}

        # Простой парсинг основных параметров
        for line in lines:
            if "Physical focal length:" in line:
                try:
                    intrinsics["focal_length_mm"] = float(line.split(":")[1].strip().split()[0])
                except:
                    pass
            elif "Capture resolution:" in line:
                try:
                    parts = line.split(":")[1].strip().split("x")
                    intrinsics["width"] = int(parts[0].strip())
                    intrinsics["height"] = int(parts[1].strip().split()[0])
                except:
                    pass

        self.camera_intrinsics = intrinsics
        print(f"Загружены параметры камеры")
        return intrinsics

    def parse_all(self):
        """Парсит все доступные файлы логов"""
        print(f"\n{'='*60}")
        print(f"Парсинг логов из: {self.log_path}")
        print(f"{'='*60}\n")

        self.parse_imu()
        self.parse_gps()
        self.parse_odom()
        self.list_camera_images()
        self.parse_camera_intrinsics()

        print(f"\n{'='*60}")
        print("Парсинг завершен")
        print(f"{'='*60}\n")

    def get_summary(self) -> Dict:
        """
        Возвращает сводку по загруженным данным.

        Returns:
            Словарь с информацией о данных
        """
        summary = {
            "imu_records": len(self.imu_data) if self.imu_data is not None else 0,
            "gps_records": len(self.gps_data) if self.gps_data is not None else 0,
            "can_frames": len(self.can_data) if self.can_data else 0,
            "wheel_speed_records": len(self.wheel_speed),
            "steering_records": len(self.steering),
            "gear_records": len(self.gear),
            "camera_images": len(self.camera_images),
            "has_intrinsics": self.camera_intrinsics is not None,
        }

        if self.imu_data is not None and len(self.imu_data) > 0:
            duration_ms = self.imu_data[-1, 0] - self.imu_data[0, 0]
            summary["duration_seconds"] = duration_ms / 1000.0

        return summary

    # ============================================================================
    # Getters для доступа к данным
    # ============================================================================

    def get_imu(self) -> Optional[np.ndarray]:
        """Возвращает IMU данные"""
        return self.imu_data

    def get_gps(self) -> Optional[np.ndarray]:
        """Возвращает GPS данные"""
        return self.gps_data

    def get_can(self) -> Optional[List[Dict]]:
        """Возвращает CAN фреймы"""
        return self.can_data

    def get_wheel_speed(self) -> List:
        """Возвращает данные скоростей колес"""
        return self.wheel_speed

    def get_steering(self) -> List:
        """Возвращает данные углов поворота руля"""
        return self.steering

    def get_gear(self) -> List:
        """Возвращает данные передач"""
        return self.gear

    def get_images(self) -> List[str]:
        """Возвращает список путей к изображениям"""
        return self.camera_images

    def get_intrinsics(self) -> Optional[Dict]:
        """Возвращает параметры камеры"""
        return self.camera_intrinsics
