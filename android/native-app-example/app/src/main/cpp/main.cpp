#include <libusb.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <pthread.h>

#define log(...) __android_log_print(ANDROID_LOG_INFO, "libusb-example", __VA_ARGS__)

void * sidethread_run(void * vctx) {
    libusb_context * ctx = (libusb_context *)vctx;
    libusb_device ** list;
    intptr_t r;

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

    return (void*)r;
}

void android_main(struct android_app * state) {
    libusb_context * ctx;
    pthread_t sidethread;
    void *sidethread_ret;
    int r;

    r = libusb_set_option(0, LIBUSB_OPTION_ANDROID_JAVAVM, state->activity->vm, 0);
    log("libusb_set_option ANDROID_JAVAVM: %s", libusb_strerror(r));

    r = libusb_init(&ctx);
    log("libusb_init: %s", libusb_strerror(r));

    r = libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG, 0);
    log("libusb_set_option LOG_LEVEL: %s", libusb_strerror(r));
    
    r = pthread_create(&sidethread, 0, sidethread_run, ctx);
    log("pthread_create: %i", r);

    r = pthread_join(sidethread, &sidethread_ret);
    r = (intptr_t)sidethread_ret;

    JNIEnv *jni_env;
    r = state->activity->vm->GetEnv((void**)&jni_env, JNI_VERSION_1_1);
    log("GetEnv after thread = %i ; JNI_EDETACHED = %i", r, JNI_EDETACHED);

    libusb_exit(ctx);
}
