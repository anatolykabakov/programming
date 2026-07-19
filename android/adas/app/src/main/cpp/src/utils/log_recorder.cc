#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "panda/panda.h"
#include "utils/can_parser.h"
#include "utils/can_logger.h"

// lsusb: Bus 003 Device 005: ID bbaa:ddcc comma.ai panda
//   Bus: 003, Device: 005
//   VID: 0xbbaa, PID: 0xddcc
//   File descriptor: 005
//   Device path: /dev/bus/usb/003/005
//   ✓ Устройство доступно

int main(int argc, char** argv)
{
  std::cout << "=== Panda Device Detection Tool ===" << std::endl;
  std::cout << "Note: To find USB device file descriptors, use:" << std::endl;
  std::cout << "  lsusb -v | grep -E '(Bus|Device|idVendor|idProduct)'" << std::endl;
  std::cout << "  or check /dev/bus/usb/*/* for available devices" << std::endl;

  // Инициализация CAN логгера
  CanLogger can_logger("log.txt");

  std::cout << "CAN logging to file: " << can_logger.getFilename() << std::endl;
  std::cout << "\n1. Searching for Panda devices using PandaUsbHandle::list()..." << std::endl;
  try {
    std::vector<std::string> panda_devices = Panda::list();

    std::unique_ptr<Panda> panda;

    if (panda_devices.empty()) {
      std::cout << "   No Panda devices found." << std::endl;
    } else {
      panda = std::make_unique<Panda>(panda_devices[0], 0);

      std::cout << "   Successfully connected to device" << std::endl;
      std::cout << "   Hardware Serial: " << panda->hw_serial() << std::endl;
      std::cout << "   Connected: " << (panda->connected() ? "Yes" : "No") << std::endl;
      std::cout << "   Comms Healthy: " << (panda->comms_healthy() ? "Yes" : "No") << std::endl;
      int i = 100000;

      while (panda->connected() && i > 0 && panda->comms_healthy()) {
        std::vector<can_frame> raw_can_data;
        panda->can_receive(raw_can_data);

        for (const auto& frame : raw_can_data) {
          can_logger.logCanFrame(frame);
        }

        if (!raw_can_data.empty()) {
          std::cout << "   Can data: size " << raw_can_data.size() << " addr: 0x" << std::hex << std::uppercase
                    << raw_can_data[0].address << " busTime: " << std::dec << raw_can_data[0].busTime
                    << " src: " << static_cast<int>(raw_can_data[0].src) << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        i--;
      }
    }

  } catch (const std::exception& e) {
    std::cout << "   Error listing Panda devices: " << e.what() << std::endl;
  }

  std::cout << "\n=== Tool completed ===" << std::endl;
  std::cout << "CAN log saved to: " << can_logger.getFilename() << std::endl;
  return 0;
}
