#!/usr/bin/env python3
"""
Android Bag Player - проигрыватель bag файлов из Android ADAS приложения
"""

import json
import zipfile
import argparse
import sys
import heapq
from pathlib import Path
from datetime import datetime, timedelta
from collections import defaultdict
from typing import Any, Dict, List, Optional, Generator, Union, Callable, Tuple
import struct
import os

# Добавляем путь к protobuf модулям
sys.path.append('/workspace/programming/android/adas/app/src/main/scripts/proto')

try:
    import bag_pb2
    import messages_pb2
    import can_pb2
    import panda_pb2
    import imu_pb2
    import gps_pb2
    import camera_intrinsics_pb2
except ImportError as e:
    print(f"Error importing protobuf modules: {e}")
    print("Make sure to run generate_proto_python.sh first")
    sys.exit(1)


class AndroidBagPlayer:
    """Проигрыватель bag файлов из Android ADAS приложения"""
    
    def __init__(self, bag_path: Union[str, Path]):
        """
        Инициализация плеера
        
        Args:
            bag_path: Путь к bag файлу (.zip архив)
            store_path: Путь для разархивирования (по умолчанию /tmp)
        """
        self.bag_path = Path(bag_path)
        # if not self.bag_path.exists():
        #     raise FileNotFoundError(f"Bag file not found: {self.bag_path}")
        
        self._record_name = os.path.split(self.bag_path)[-1]
        self._record_path = Path(self.bag_path)
        
        self._topics_info = {}
        self._topics_parts = defaultdict(list)
        
        # Коллбеки для топиков
        self._callbacks: Dict[str, Callable[[str, Any, int], None]] = {}
        self._enabled_topics: set = set()
        
        self._load_bag()
    
    def __del__(self):
        """Деструктор для автоматической очистки"""
        # self.cleanup()
    
    def _load_bag(self):
        """Загружает bag файл и извлекает данные"""
        print(f"Loading bag file: {self.bag_path}")
        
        # Разархивируем bag файл
        self._unarchive_bag()
        
        # Загружаем информацию о топиках
        self._load_topics_info()
        
        print(f"Loaded {len(self._topics_parts)} topics")
        for topic in self._topics_parts:
            file_count = len(self._topics_parts[topic])
            print(f"  {topic}: {file_count} files")
    
    def _unarchive_bag(self):
        """Разархивирует bag файл"""
        if self._record_path.exists():
            print(f"Directory {self._record_path} already exists. Using existing files.")
            return
        
        print(f"Extracting bag file to {self._record_path}")
        self._record_path.mkdir(parents=True, exist_ok=True)
        
        with zipfile.ZipFile(self.bag_path, 'r') as zip_file:
            zip_file.extractall(self._record_path)
    
    def _load_topics_info(self):
        """Загружает информацию о топиках из разархивированной директории"""
        # Сканируем директорию на предмет .bin файлов
        for item in self._record_path.rglob('*.bin'):
            # Извлекаем имя топика из пути
            relative_path = item.relative_to(self._record_path)
            parts = relative_path.parts
            
            if len(parts) >= 2:
                topic_name = parts[0]  # Первая часть пути - имя топика
                self._topics_parts[topic_name].append(item)
        
        # Инициализируем информацию о топиках
        for topic_name, files in self._topics_parts.items():
            self._topics_info[topic_name] = {
                'files': [str(f) for f in files],
                'message_count': 0,  # Будет подсчитано при первом обращении
                'total_size': 0      # Будет подсчитано при первом обращении
            }
    
    def _parse_bag_file(self, raw_data: bytes) -> List[Dict]:
        """Парсит bag файл и возвращает список сообщений"""
        try:
            bag = bag_pb2.Bag()
            bag.ParseFromString(raw_data)
            
            messages = []
            for zmq_msg in bag.messages:
                msg_data = {
                    'timestamp': zmq_msg.timestamp,
                    'topic': zmq_msg.topic,
                    'message': self._parse_message(zmq_msg)
                }
                messages.append(msg_data)
            
            return messages
            
        except Exception as e:
            print(f"Error parsing bag file: {e}")
            return []
    
    def _fix_topic_name(self, topic_name: str) -> str:
        """Преобразует имя топика для использования в качестве имени директории"""
        return topic_name.replace("/", "__")
    
    def _unfix_topic_name(self, dir_name: str) -> str:
        """Преобразует имя директории обратно в имя топика"""
        return dir_name.replace("__", "/")
    
    def _parse_message(self, zmq_msg) -> Any:
        """Парсит конкретное сообщение в зависимости от топика"""
        if zmq_msg.HasField('camera_image'):
            return zmq_msg.camera_image
        elif zmq_msg.HasField('imu_data'):
            return zmq_msg.imu_data
        elif zmq_msg.HasField('gps_location'):
            return zmq_msg.gps_location
        elif zmq_msg.HasField('gps_data'):
            return zmq_msg.gps_data
        elif zmq_msg.HasField('can_data'):
            return zmq_msg.can_data
        elif zmq_msg.HasField('panda_health'):
            return zmq_msg.panda_health
        elif zmq_msg.HasField('camera_intrinsics'):
            return zmq_msg.camera_intrinsics
        else:
            return None
    
    @property
    def topics(self) -> List[str]:
        """Возвращает список доступных топиков"""
        return [self._unfix_topic_name(dir_name) for dir_name in self._topics_parts.keys()]
    
    def add_callback(
        self,
        topic_name: str,
        callback: Callable[[str, Any, int], None],
    ) -> None:
        """
        Добавляет коллбек для конкретного топика.

        Args:
            topic_name: Имя топика для подписки
            callback: Функция для вызова при получении сообщения
        """
        self._callbacks[topic_name] = callback
        self._enabled_topics.add(topic_name)
    
    def remove_callback(self, topic_name: str) -> None:
        """
        Удаляет коллбек для конкретного топика.

        Args:
            topic_name: Имя топика для отписки
        """
        if topic_name in self._callbacks:
            del self._callbacks[topic_name]
            self._enabled_topics.discard(topic_name)
    
    def clear_callbacks(self) -> None:
        """Удаляет все коллбеки."""
        self._callbacks.clear()
        self._enabled_topics.clear()
    
    def get_enabled_topics(self) -> List[str]:
        """Возвращает список топиков с зарегистрированными коллбеками."""
        return list(self._enabled_topics)
    
    def single_type_generator(
        self,
        topic_name: str,
        start_time: Optional[int] = None,
        end_time: Optional[int] = None,
    ) -> Generator[Any, None, None]:
        """Генерирует все сообщения для топика.

        Args:
            topic_name: Топик для генерации сообщений
            start_time: Опциональное время начала в миллисекундах
            end_time: Опциональное время окончания в миллисекундах
        """
        for timestamp, message in self.single_type_generator_with_ts(topic_name, start_time, end_time):
            yield message
    
    def single_type_generator_with_ts(
        self,
        topic_name: str,
        start_time: Optional[int] = None,
        end_time: Optional[int] = None,
    ) -> Generator[Tuple[int, Any], None, None]:
        """Генерирует сообщения с временными метками для топика.

        Args:
            topic_name: Топик для генерации сообщений
            start_time: Опциональное время начала в миллисекундах
            end_time: Опциональное время окончания в миллисекундах
        """
        dir_name = self._fix_topic_name(topic_name)
        if dir_name not in self._topics_parts:
            return
        
        for file_path in self._topics_parts[dir_name]:
            try:
                with open(file_path, 'rb') as f:
                    raw_data = f.read()
                    messages = self._parse_bag_file(raw_data)
                    
                    for msg in messages:
                        timestamp = msg['timestamp']
                        
                        # Пропускаем сообщения вне временного диапазона
                        if start_time is not None and timestamp < start_time:
                            continue
                        if end_time is not None and timestamp > end_time:
                            break  # Предполагаем, что сообщения упорядочены по времени
                        
                        yield (timestamp, msg['message'])
                        
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
    
    def play(
        self,
        start_time: Optional[int] = None,
        end_time: Optional[int] = None,
    ) -> None:
        """
        Проигрывает все включенные топики в хронологическом порядке.

        Args:
            start_time: Опциональное время начала в миллисекундах
            end_time: Опциональное время окончания в миллисекундах
        """
        if not self._enabled_topics:
            print("No callbacks registered. Use add_callback() to register callbacks first.")
            return

        # Создаем генераторы для всех включенных топиков
        topic_generators = {}
        for topic_name in self._enabled_topics:
            topic_generators[topic_name] = self.single_type_generator_with_ts(
                topic_name, start_time, end_time
            )

        # Инициализируем очередь приоритетов с первым сообщением из каждого топика
        message_queue: List[Tuple[int, str, Any]] = []
        active_generators: Dict[str, Generator[Tuple[int, Any], None, None]] = {}

        # Получаем первое сообщение из каждого топика
        for topic_name, generator in topic_generators.items():
            try:
                timestamp, message = next(generator)
                heapq.heappush(message_queue, (timestamp, topic_name, message))
                active_generators[topic_name] = generator
            except StopIteration:
                # Топик не имеет сообщений в временном диапазоне
                continue

        # Обрабатываем сообщения в хронологическом порядке
        while message_queue:
            timestamp, topic_name, message = heapq.heappop(message_queue)

            # Вызываем коллбек для этого топика
            if topic_name in self._callbacks:
                try:
                    self._callbacks[topic_name](topic_name, message, timestamp)
                except Exception as e:
                    print(f"Error in callback for topic '{topic_name}': {e}")

            # Получаем следующее сообщение из этого топика
            if topic_name in active_generators:
                try:
                    next_timestamp, next_message = next(active_generators[topic_name])
                    heapq.heappush(
                        message_queue,
                        (next_timestamp, topic_name, next_message),
                    )
                except StopIteration:
                    # Больше нет сообщений для этого топика
                    del active_generators[topic_name]
    
    def get_topic_info(self, topic: str) -> Dict:
        """Возвращает информацию о топике"""
        dir_name = self._fix_topic_name(topic)
        if dir_name not in self._topics_info:
            return {}
        return self._topics_info[dir_name].copy()
    
    def get_topic_msgs(self, topic_name: str) -> List[Tuple[int, Any]]:
        messages = []
        for timestamp, msg in self.single_type_generator_with_ts(topic_name):
            messages.append((timestamp, msg))
        return messages
    
    def get_time_range(self) -> tuple:
        """Возвращает временной диапазон (start, end) в миллисекундах"""
        all_timestamps = []
        for topic in self.topics:
            # Получаем временные метки из генератора
            for timestamp, _ in self.single_type_generator_with_ts(topic):
                all_timestamps.append(timestamp)
        
        if not all_timestamps:
            return (0, 0)
        
        return (min(all_timestamps), max(all_timestamps))
    
    def get_duration(self) -> timedelta:
        """Возвращает продолжительность записи"""
        start_time, end_time = self.get_time_range()
        if start_time == 0 and end_time == 0:
            return timedelta(0)
        
        start_dt = datetime.fromtimestamp(start_time / 1000)
        end_dt = datetime.fromtimestamp(end_time / 1000)
        return end_dt - start_dt
    
    def print_summary(self):
        """Выводит сводку по bag файлу"""
        print(f"\n=== Bag File Summary ===")
        print(f"File: {self.bag_path}")
        print(f"Topics: {len(self.topics)}")
        
        start_time, end_time = self.get_time_range()
        if start_time > 0 and end_time > 0:
            start_dt = datetime.fromtimestamp(start_time / 1000)
            end_dt = datetime.fromtimestamp(end_time / 1000)
            duration = self.get_duration()
            print(f"Time range: {start_dt.strftime('%Y-%m-%d %H:%M:%S')} - {end_dt.strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"Duration: {duration}")
        
        print(f"\n=== Topics ===")
        for topic in self.topics:
            info = self.get_topic_info(topic)
            print(f"{topic}:")
            print(f"  Files: {len(info['files'])}")


def main():
    """Главная функция для запуска плеера"""
    parser = argparse.ArgumentParser(description="Android Bag Player")
    parser.add_argument("bag_file", help="Path to bag file (.zip)")
    parser.add_argument("--topic", help="Show messages for specific topic")
    parser.add_argument("--summary", action="store_true", help="Show bag file summary")
    parser.add_argument("--extract-images", action="store_true", help="Extract camera images")
    
    args = parser.parse_args()
    
    try:
        player = AndroidBagPlayer(args.bag_file)
        
        if args.summary:
            player.print_summary()
        elif args.topic:
            if args.topic not in player.topics:
                print(f"Topic '{args.topic}' not found. Available topics: {player.topics}")
                return
            
            print(f"\n=== Messages for topic: {args.topic} ===")
            count = 0
            for timestamp, message in player.single_type_generator_with_ts(args.topic):
                timestamp_dt = datetime.fromtimestamp(timestamp / 1000)
                print(f"{count}: {timestamp_dt.strftime('%H:%M:%S.%f')[:-3]} - {type(message).__name__}")
                
                # Показываем содержимое сообщения
                if hasattr(message, 'SerializeToString'):
                    print(f"  Message: {message}")
                
                count += 1
                if count >= 1000:  # Показываем только первые 10 сообщений
                    print("... (showing first 10 messages)")
                    break
        
        elif args.extract_images:
            # Извлекаем изображения камеры
            if 'sensors/camera/image' in player.topics:
                output_dir = args.bag_file / Path("extracted_images")
                output_dir.mkdir(exist_ok=True)
                
                count = 0
                for timestamp, message in player.single_type_generator_with_ts('sensors/camera/image'):
                    if message and hasattr(message, 'image_data'):
                        image_data = message.image_data
                        timestamp_dt = datetime.fromtimestamp(timestamp / 1000)
                        filename = output_dir / f"{timestamp}.jpg"
                        
                        with open(filename, 'wb') as f:
                            f.write(image_data)
                        
                        print(f"Extracted: {filename}")
                        count += 1
                
                print(f"Extracted {count} images to {output_dir}")
            else:
                print("No camera images found in bag file")
        
        else:
            # Показываем доступные топики
            player.print_summary()
            
            # Пример использования с коллбеками
            if player.topics:
                print(f"\n=== Example with callbacks ===")
                
                def callback(topic_name: str, message: Any, timestamp: int):
                    print(f"Received message from {topic_name} at {timestamp}: {type(message).__name__}")
                
                # Добавляем коллбек для первого топика
                first_topic = player.topics[0]
                player.add_callback(first_topic, callback)
                
                # Проигрываем первые 5 сообщений
                print(f"Playing first 5 messages from {first_topic}...")
                count = 0
                for timestamp, message in player.single_type_generator_with_ts(first_topic):
                    callback(first_topic, message, timestamp)
                    count += 1
                    if count >= 5:
                        break
    
    except Exception as e:
        import traceback
        print(f"Error: {e}")
        print("Full traceback:")
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
