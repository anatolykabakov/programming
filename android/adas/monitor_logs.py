#!/usr/bin/env python3
"""
Script to monitor ADAS logs from Android device
"""

import os
import time
import subprocess
import json
from datetime import datetime

def run_adb_command(cmd):
    """Run ADB command and return output"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.stdout.strip()
    except Exception as e:
        print(f"Error running command: {e}")
        return ""

def get_log_files():
    """Get list of log files from device"""
    cmd = "adb shell ls /sdcard/adas_logs/"
    output = run_adb_command(cmd)
    if output:
        return [line.strip() for line in output.split('\n') if line.strip()]
    return []

def tail_log_file(log_file, lines=10):
    """Get last N lines of a log file"""
    cmd = f"adb shell tail -n {lines} /sdcard/adas_logs/{log_file}"
    return run_adb_command(cmd)

def get_file_size(log_file):
    """Get file size"""
    cmd = f"adb shell stat -c %s /sdcard/adas_logs/{log_file}"
    size = run_adb_command(cmd)
    try:
        return int(size)
    except:
        return 0

def format_size(size_bytes):
    """Format file size in human readable format"""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.1f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.1f} MB"

def monitor_logs():
    """Main monitoring function"""
    print("ADAS ZMQ Logger Monitor")
    print("=" * 50)
    
    while True:
        try:
            # Clear screen
            os.system('clear' if os.name == 'posix' else 'cls')
            
            print(f"ADAS ZMQ Logger Monitor - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            print("=" * 50)
            
            # Get log files
            log_files = get_log_files()
            
            if not log_files:
                print("No log files found in /sdcard/adas_logs/")
                print("Make sure the app is running and logging is enabled.")
                time.sleep(5)
                continue
            
            print(f"Found {len(log_files)} log files:")
            print()
            
            # Show file sizes and recent content
            for log_file in sorted(log_files):
                size = get_file_size(log_file)
                print(f"📄 {log_file} ({format_size(size)})")
                
                # Show last 3 lines
                content = tail_log_file(log_file, 3)
                if content:
                    for line in content.split('\n')[-3:]:
                        if line.strip():
                            print(f"   {line.strip()}")
                print()
            
            # Check for specific log types
            log_types = {
                'camera_state.log': '📷 Camera State',
                'camera_buffer.log': '🖼️ Camera Buffer', 
                'gps_data.log': '📍 GPS Data',
                'imu_data.log': '📱 IMU Data',
                'counters.log': '📊 Message Counters'
            }
            
            print("Log Status:")
            for log_file, description in log_types.items():
                if log_file in log_files:
                    size = get_file_size(log_file)
                    print(f"  ✅ {description}: {format_size(size)}")
                else:
                    print(f"  ❌ {description}: Not found")
            
            print()
            print("Press Ctrl+C to exit")
            print("Refreshing in 5 seconds...")
            
            time.sleep(5)
            
        except KeyboardInterrupt:
            print("\nMonitoring stopped.")
            break
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(5)

def download_logs():
    """Download all logs to local directory"""
    print("Downloading logs...")
    
    # Create local logs directory
    local_dir = "adas_logs"
    os.makedirs(local_dir, exist_ok=True)
    
    # Get log files
    log_files = get_log_files()
    
    if not log_files:
        print("No log files found to download.")
        return
    
    for log_file in log_files:
        print(f"Downloading {log_file}...")
        cmd = f"adb pull /sdcard/adas_logs/{log_file} {local_dir}/"
        run_adb_command(cmd)
    
    print(f"Logs downloaded to {local_dir}/ directory")

def show_help():
    """Show help information"""
    print("ADAS ZMQ Logger Monitor")
    print("=" * 30)
    print("Usage: python3 monitor_logs.py [command]")
    print()
    print("Commands:")
    print("  monitor    - Monitor logs in real-time (default)")
    print("  download   - Download all logs to local directory")
    print("  help       - Show this help message")
    print()
    print("Make sure ADB is installed and device is connected.")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1:
        command = sys.argv[1].lower()
        if command == "download":
            download_logs()
        elif command == "help":
            show_help()
        else:
            print(f"Unknown command: {command}")
            show_help()
    else:
        monitor_logs()
