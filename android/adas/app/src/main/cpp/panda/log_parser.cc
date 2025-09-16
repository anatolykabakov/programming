#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include "can_parser.h"
#include "can_logger.h"

// Функция для вывода информации о CAN фрейме
void print_frame(int frame_number, const can_frame& frame) {
    std::cout << "Frame " << frame_number << ": "
              << "Addr=0x" << std::hex << std::uppercase << frame.address << std::dec
              << ", Len=" << frame.dat.size()
              << ", Data=";

    for (size_t i = 0; i < frame.dat.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(static_cast<uint8_t>(frame.dat[i])) << std::dec;
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    // std::cout << "=== CAN Log Parser ===" << std::endl;
    
    // Проверяем аргументы командной строки
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <log_file> [dbc_file]" << std::endl;
        std::cout << "  log_file: Path to CAN log file" << std::endl;
        std::cout << "  dbc_file: Path to DBC file (optional, defaults to vw_meb.dbc)" << std::endl;
        return 1;
    }
    
    std::string log_file = argv[1];
    std::string dbc_file = (argc > 2) ? argv[2] : "/workspace/programming/android/adas/opendbc/opendbc/dbc/vw_meb.dbc";
    
    // std::cout << "Log file: " << log_file << std::endl;
    // std::cout << "DBC file: " << dbc_file << std::endl;
    
    // Инициализация DBC парсера
    DBSParser dbc_parser(dbc_file);
    
    // Открываем лог файл
    std::ifstream log_stream(log_file);
    if (!log_stream.is_open()) {
        std::cerr << "Error: Cannot open log file: " << log_file << std::endl;
        return 1;
    }
    
    std::string line;
    int total_frames = 0;
    int parsed_frames = 0;
    int esc51_frames = 0;
    int valid_speeds = 0;
    
    // std::cout << "\nProcessing log file..." << std::endl;
    
    while (std::getline(log_stream, line)) {
        total_frames++;
        
        // Используем parseCanFrameFromLine из логгера
        auto can_frame_with_timestamp_opt = CanLogger::parseCanFrameFromLine(line);
        if (!can_frame_with_timestamp_opt.has_value()) {
            continue;
        }
        
        parsed_frames++;
        const auto& can_frame_with_timestamp = can_frame_with_timestamp_opt.value();
        const auto& can_frame = can_frame_with_timestamp.frame;
        const auto& timestamp = can_frame_with_timestamp.timestamp;
        
        // Выводим информацию о CAN фрейме
        // print_frame(parsed_frames, can_frame);
        
        // Проверяем, является ли это сообщением ESC_51 (0xFC)
        if (can_frame.address == 0xFC) {
            esc51_frames++;
            
            auto esc51_msg = dbc_parser.getMessage(can_frame.address);
            if (esc51_msg.has_value()) {
                // std::cout << "  ESC_51 message found: " << esc51_msg->name 
                //           << " (length=" << (int)esc51_msg->length << ")" << std::endl;
                
                // Извлекаем скорости колес
                auto vr_speed = dbc_parser.extractSignal(can_frame, "VR_Radgeschw");
                auto vl_speed = dbc_parser.extractSignal(can_frame, "VL_Radgeschw");
                auto hr_speed = dbc_parser.extractSignal(can_frame, "HR_Radgeschw");
                auto hl_speed = dbc_parser.extractSignal(can_frame, "HL_Radgeschw");
                
                if (vr_speed.has_value() && vl_speed.has_value() && hr_speed.has_value() && hl_speed.has_value()) {
                    std::cout << timestamp << "," << vr_speed.value() << "," << vl_speed.value() << "," << hr_speed.value() << "," << hl_speed.value() << std::endl;
                    valid_speeds++;
                } else {
                    std::cout << "  No wheel speed signals found or extraction failed" << std::endl;
                }
            } else {
                std::cout << "  ESC_51 message not found in DBC" << std::endl;
            }
        }
    }
    
    // Статистика
    std::cout << "\n=== Statistics ===" << std::endl;
    std::cout << "Total lines processed: " << total_frames << std::endl;
    std::cout << "Valid CAN frames: " << parsed_frames << std::endl;
    std::cout << "ESC_51 frames: " << esc51_frames << std::endl;
    std::cout << "Valid wheel speeds extracted: " << valid_speeds << std::endl;
    
    return 0;
}
