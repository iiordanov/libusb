/*
 * android_jni interface
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 * Copyright © 2001 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBUSB_ANDROID_JNI_H
#define LIBUSB_ANDROID_JNI_H

#include "libusbi.h"

struct android_jni_devices;

/* from jni.h */
typedef const struct JNINativeInterface *JNIEnv;
typedef const struct JNIInvokeInterface *JavaVM;
typedef void *jobject;

/* Converts a JNIEnv pointer into a JavaVM pointer, which is needed for the other calls.
 *
 * JNIEnv pointers are local to their context and shouldn't be stashed, but they are
 * more easy for a user to provide.
 */
int android_jni_javavm(JNIEnv *jni_env, JavaVM **javavm);

/* Detects whether or not the platform supports usb host mode. */
int android_jni_detect_usbhost(JavaVM *javavm, int *has_usbhost);

/* Prepares to iterate all connected devices. */
int android_jni_devices_alloc(JavaVM *javavm, struct android_jni_devices **devices);

/* Iterates through connected devices.
 *
 * The device jobject should be freed with android_jni_globalunref().
 *
 * Returns LIBUSB_ERROR_NOT_FOUND when all devices have been enumerated.
 */
int android_jni_devices_next(struct android_jni_devices *devices, jobject *device, uint8_t *busnum, uint8_t *devaddr);

/* Frees a device iteration structure. */
void android_jni_devices_free(struct android_jni_devices *devices);

/* Connects to a device.
 *
 * The connection jobject should be freed with android_jni_globalunref().
 *
 * The descriptors buffer should be freed with free().
 *
 * If permission is needed, LIBUSB_ERROR_ACCESS is returned and permission is requested of the user,
 * using an intent with the action "libusb.android.USB_PERMISSION".
 *
 * For further information on this intent, which is dispatched by UsbManager.requestPermission, see:
 *  https://developer.android.com/guide/topics/connectivity/usb/host#permission-d
 *
 *  TODO: separate the permission calls out to ease maintenance
 */
int android_jni_connect(JavaVM *javavm, jobject device, jobject *connection, int *fd, int8_t **descriptors, int *descriptors_len);

/* Disconnects from a device. */
int android_jni_disconnect(JavaVM *javavm, jobject connection);

/* Frees a global reference to an object. */
void android_jni_globalunref(JavaVM *javavm, jobject *object);

#endif
