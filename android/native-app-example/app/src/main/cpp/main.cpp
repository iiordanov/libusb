#include <libusb.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define log(...) __android_log_print(ANDROID_LOG_INFO, "libusb-example", __VA_ARGS__)

void android_main(struct android_app * state) {
    libusb_context * ctx;
    libusb_device ** list;
    int r;

    r = libusb_set_option(0, LIBUSB_OPTION_ANDROID_JNIENV, state->activity->env, 0);
    log("libusb_set_option ANDROID_JNIENV: %s", libusb_strerror(r));

    r = libusb_init(&ctx);
    log("libusb_init: %s", libusb_strerror(r));

    r = libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG, 0);
    log("libusb_set_option LOG_LEVEL: %s", libusb_strerror(r));

    r = libusb_get_device_list(ctx, &list);
    log("libusb_get_device_list: %s", libusb_strerror(r > 0 ? LIBUSB_SUCCESS : r));

    for (int i = 0; i < r; ++ i) {
        libusb_device *dev = list[i];
        uint8_t bus = libusb_get_bus_number(dev);
        uint8_t port = libusb_get_port_number(dev);
        uint8_t addr = libusb_get_device_address(dev);
        struct libusb_device_descriptor desc;
        libusb_device_handle *dev_handle;

        log("Found bus=%u port=%u addr=%u", bus, port, addr);
        r = libusb_get_device_descriptor(dev, &desc);
        if (r == LIBUSB_SUCCESS) {
            log("USB version: %u.%u.%u", desc.bcdUSB>>8, (desc.bcdUSB&0xF0)>>4, desc.bcdUSB&0xF);
            log("Device class: %u", desc.bDeviceClass);
            log("Device subclass: %u", desc.bDeviceSubClass);
            log("Device protocol: %u", desc.bDeviceProtocol);
            log("Vendor: %04x", desc.idVendor);
            log("Product: %04x", desc.idProduct);
            log("Configuration count: %u", desc.bNumConfigurations);
        }

        r = libusb_open(dev, &dev_handle);
        log("libusb_open: %s", libusb_strerror(r));

        if (r == LIBUSB_SUCCESS) {
            libusb_close(dev_handle);
        }
    }

    libusb_free_device_list(list, 1);
    

    libusb_exit(ctx);
}
