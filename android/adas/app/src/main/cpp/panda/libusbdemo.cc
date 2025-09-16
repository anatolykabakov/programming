#include <iostream>
#include <libusb-1.0/libusb.h> // Include the libusb header

int main() {
    libusb_context* ctx = NULL; // a libusb session
    libusb_device_handle* dev_handle = NULL; // a device handle

    // Initialize libusb
    int r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Error initializing libusb: " << libusb_error_name(r) << std::endl;
        return 1;
    }

    // Set debugging output to max level (optional)
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    libusb_device **dev_list = NULL;
    int num_devices = libusb_get_device_list(ctx, &dev_list);

    std::cout << "Number of devices: " << num_devices << std::endl;

    for (size_t i = 0; i < num_devices; ++i) {
        libusb_device_descriptor desc;
        libusb_get_device_descriptor(dev_list[i], &desc);
        std::cout << desc.idVendor << " " << desc.idProduct << std::endl;
        // if (desc.idVendor == 0xbbaa && desc.idProduct == 0xddcc) {
        // }
    }

    // Open a device by Vendor ID and Product ID
    // Replace 0xVVVV and 0xPPPP with your device's actual VID and PID
    dev_handle = libusb_open_device_with_vid_pid(ctx, 0xbbaa, 0xddcc); 
    if (dev_handle == NULL) {
        std::cerr << "Could not find/open device with VID:PID 0xbbaa:0xddcc" << std::endl;
        libusb_exit(ctx);
        return 1;
    }

    std::cout << "Device found and opened successfully." << std::endl;

    // Perform operations with the device here (e.g., claiming interface, reading/writing data)
    // ...

    // Close the device handle
    libusb_close(dev_handle);

    // Deinitialize libusb
    libusb_exit(ctx);

    std::cout << "Libusb exited successfully." << std::endl;

    return 0;
}