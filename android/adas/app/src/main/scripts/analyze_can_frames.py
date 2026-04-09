#!/usr/bin/env python3
"""
Script to analyze CAN frames from bag files using OpenDBC parser with VW MEB DBC file.
"""

import sys
import os
import csv
from dataclasses import dataclass
from typing import List, Optional, Dict, Any
import argparse

# Add OpenDBC to path
sys.path.append("/workspace/programming/android/adas/opendbc")
sys.path.append("/workspace/programming/android/adas/app/src/main/scripts")

# Add bag player to path
from android_bag_player import AndroidBagPlayer

try:
    from opendbc.can.parser import CANParser
    from opendbc.can.dbc import DBC
except ImportError as e:
    print(f"Error importing OpenDBC: {e}")
    print("Make sure pycapnp is installed: pip install pycapnp")
    sys.exit(1)

# Initialize OpenDBC parser
messages = [
    # Steering and control
    ("LWI_01", 100),  # Steering wheel angle (0x86) - 100 Hz
    ("LH_EPS_03", 100),  # Electric Power Steering (0x9F) - 100 Hz
    ("HCA_01", 10),  # Heading Control Assist (0x126) - 10 Hz
    ("HCA_03", 10),  # Alternative curvature control (0x303) - 10 Hz
    ("QFK_01", 100),  # Full steering control (0x13D) - 100 Hz
    # ACC and cruise control
    ("GRA_ACC_01", 10),  # ACC controls (0x12B) - 10 Hz
    ("ACC_18", 10),  # ACC status (0x14D) - 10 Hz
    ("MEB_ACC_01", 10),  # MEB ACC (0x300) - 10 Hz
    # Motor and powertrain
    ("Motor_51", 10),  # Motor data (0x10B) - 10 Hz
    ("Motor_14", 10),  # Motor brake status (0x3BE) - 10 Hz
    # Vehicle dynamics
    ("ESC_51", 100),  # ESC wheel speeds (0xFC) - 100 Hz
    ("ESP_21", 10),  # ESP data (0xFD) - 10 Hz
    # Safety and assistance
    ("EA_01", 1),  # EA mitigation (0x1A4) - 1 Hz
    ("EA_02", 1),  # EA mitigation (0x1F0) - 1 Hz
    ("KLR_01", 10),  # Capacitive steering wheel (0x25D) - 10 Hz
    ("TA_01", 10),  # Travel Assist status (0x26B) - 10 Hz
    ("LDW_02", 10),  # Lane departure warning (0x397) - 10 Hz
    # Gateway and communication
    ("Gateway_73", 10),  # Gateway gear info (0x3DC) - 10 Hz
]

@dataclass
class CanFrame:
    timestamp: str
    address: int
    bus_time: str
    src: str
    data_size: int
    raw_data: List[str]
    data_bytes: bytes
    parsed_data: Dict[str, Any]

def parse_can_message(message, timestamp, can_parser) -> List[CanFrame]:
    """
    Parse CAN data from protobuf message
    
    Args:
        message: Protobuf message containing CAN data (CANData)
        timestamp: Message timestamp
        
    Returns:
        List of CanFrame objects
    """
    can_frames = []
    
    try:
        # Check if message has frames (CANData structure)
        if hasattr(message, 'frames') and message.frames:
            for frame in message.frames:
                if hasattr(frame, 'address') and hasattr(frame, 'data'):
                    address = frame.address
                    data_bytes_obj = frame.data
                    
                    # Convert bytes to hex string for display
                    hex_data = data_bytes_obj.hex()
                    data_bytes = [hex_data[i:i+2] for i in range(0, len(hex_data), 2)]
                    
                    # Parse signals using OpenDBC parser
                    parsed_data = {}
                    try:
                        can_parser.update([(address, [(address, data_bytes_obj, 0)])], sendcan=False)
                        if address in can_parser.vl:
                            parsed_data = can_parser.vl[address].copy()
                    except Exception as e:
                        parsed_data = {"parse_error": str(e)}
                    
                    can_frame = CanFrame(
                        timestamp=str(timestamp),
                        address=address,
                        bus_time=str(frame.bus_time) if hasattr(frame, 'bus_time') else str(timestamp),
                        src=str(frame.src) if hasattr(frame, 'src') else "bag",
                        data_size=len(data_bytes_obj),
                        raw_data=data_bytes,
                        data_bytes=data_bytes_obj,
                        parsed_data=parsed_data,
                    )
                    can_frames.append(can_frame)
            
    except Exception as e:
        print(f"Error parsing CAN message: {e}")
    
    return can_frames

def save_to_csv_file(headers, rows, csv_file):
    with open(csv_file, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=headers)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)
    print(f"Saved {len(rows)} rows to {csv_file}")

def print_unique_can_frames(can_frames, dbc):
    unique_addresses = set()
    for address in can_frames:
        for can_frame in can_frames[address]:
            unique_addresses.add(can_frame.address)
    print(f"Unique addresses: {len(unique_addresses)}")
    for address in unique_addresses:
        message_name = dbc.msgs[address].name if address in dbc.msgs else "Unknown"
        print(f"Address: {address} {message_name} - {len(can_frames[address])} frames")

def save_can_rx_to_plotjuggler(can_rx_msgs, dbc, can_parser, start_time, can_rx_csv):
    can_frames = {}
    rows = []
    for timestamp, message in can_rx_msgs:
        frames = parse_can_message(message, timestamp, can_parser)
        for can_frame in frames:
            if can_frame.address not in can_frames:
                can_frames[can_frame.address] = []
            can_frames[can_frame.address].append(can_frame)
    print_unique_can_frames(can_frames, dbc)
    headers, rows = convert_can_frames_to_csv(can_frames, start_time)
    save_to_csv_file(headers, rows, can_rx_csv)

def convert_can_frames_to_csv(can_frames, start_time=None):
    """Save CAN frames signals to CSV for PlotJuggler analysis"""
    print("Saving CAN frames signals to CSV...")
    
    all_signals = set()
    for address in can_frames:
        for can_frame in can_frames[address]:
            for signal_name in can_frame.parsed_data.keys():
                if signal_name != 'parse_error':
                    all_signals.add(f'{address}/{signal_name}')
    
    headers = ['timestamp'] + sorted(all_signals)

    data = []
    for address in can_frames:
        for can_frame in can_frames[address]:
            dt = (float(can_frame.timestamp) - float(start_time)) / 1e3
            row = {'timestamp': f"{dt:.3f}"}       # Convert from milliseconds to seconds
            
            # Add all signals for this frame
            for signal_name, signal_value in can_frame.parsed_data.items():
                if signal_name != 'parse_error':
                    row[f'{address}/{signal_name}'] = signal_value
            
            # Fill missing signals with 0
            for signal in all_signals:
                if signal not in row:
                    row[signal] = 0

            data.append(row)
    return headers, data

def save_panda_health_to_plotjuggler(panda_health_msgs, start_time, panda_health_csv):
    headers, rows = convert_panda_health_to_csv(panda_health_msgs, start_time)
    save_to_csv_file(headers, rows, panda_health_csv)

def convert_panda_health_to_csv(panda_health_msgs, start_time):
    headers = ['timestamp', 'uptime_pkt', 'controls_allowed', 'safety_mode', 'safety_param',
                'fault_status', 'voltage_mv', 'current_ma', 'power_save_enabled',
                'tx_blocked', 'tx_overflow', 'rx_invalid', 'rx_overflow', 'rx_checks_invalid',
                'faults_pkt', 'spi_error_count', 'ignition_line', 'ignition_can',
                'car_harness_status', 'heartbeat_lost', 'alternative_experience',
                'interrupt_load', 'fan_power', 'sbu1_voltage_mv', 'sbu2_voltage_mv', 'som_reset_triggered']
    data = []
    for timestamp, message in panda_health_msgs:
        # Calculate relative timestamp in seconds
        relative_time = (float(timestamp) - float(start_time)) / 1e3
        row = {'timestamp': relative_time}
        for signal in headers[1:]:  # Skip timestamp
            row[signal] = getattr(message, signal, 0)
        data.append(row)
    return headers, data

def main():
    parser = argparse.ArgumentParser(description="Analyze CAN frames from bag files with OpenDBC")
    parser.add_argument("bag_dir", type=str, help="Path to the bag directory")
    parser.add_argument(
        "--address", "-a", type=int, help="Show details for specific CAN address (hex format)"
    )
    parser.add_argument(
        "--dbc", "-d", type=str, default="/workspace/programming/android/adas/app/src/main/assets/vw_meb.dbc", help="Path to the DBC file"
    )
    parser.add_argument(
        "--topic", "-t", type=str, help="Topic to extract data from"
    )
    args = parser.parse_args()

    dbc = DBC(args.dbc)
    can_parser = CANParser(args.dbc, messages, 0)
    player = AndroidBagPlayer(args.bag_dir)    
    start_time, _ = player.get_time_range()
    print(f"Start time: {start_time}")
    # Handle Panda health extraction
    if args.topic == 'panda/health':
        panda_health_msgs = player.get_topic_msgs(args.topic)
        panda_health_csv = os.path.join(args.bag_dir, "panda_health.csv")
        save_panda_health_to_plotjuggler(panda_health_msgs, start_time, panda_health_csv)
    elif args.topic == 'can/rx':
        can_rx_msgs = player.get_topic_msgs(args.topic)
        can_rx_csv = os.path.join(args.bag_dir, "can_rx.csv")
        save_can_rx_to_plotjuggler(can_rx_msgs, dbc, can_parser, start_time, can_rx_csv)
    else:
        print(f"Topic {args.topic} not supported")

if __name__ == "__main__":
    main()