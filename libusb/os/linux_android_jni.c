/* -*- Mode: C; c-basic-offset:8 ; indent-tabs-mode:t -*- */
/*
 * Android jni interface for libusb
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

#include "linux_android_jni.h"
#include "libusb.h"

#include <jni.h>


static jclass Collection, HashMap, Iterator;
static jmethodID Collection_iterator;
static jmethodID HashMap_values;
static jmethodID Iterator_hasNext, Iterator_next;

static jclass ActivityThread, Context, PackageManager, UsbDevice, UsbManager;
static jmethodID ActivityThread__currentActivityThread, ActivityThread_getApplication;
static jfieldID Context__USB_SERVICE;
static jmethodID Context_getPackageManager, Context_getSystemService;
static jfieldID PackageManager__FEATURE_USB_HOST;
static jmethodID PackageManager_hasSystemFeature;
static jmethodID UsbDevice_getDeviceId;
static jmethodID UsbManager_getDeviceList;

int android_jni_javavm(JNIEnv *jni_env, JavaVM **javavm)
{
	int r = (*jni_env)->GetJavaVM(jni_env, javavm);
	if (r != JNI_OK)
		return LIBUSB_ERROR_OTHER;

	Collection = (*jni_env)->FindClass(jni_env, "java/util/Collection");
	Collection_iterator = (*jni_env)->GetMethodID(jni_env, Collection, "iterator", "()Ljava/util/Iterator;");

	HashMap = (*jni_env)->FindClass(jni_env, "java/util/HashMap");
	HashMap_values = (*jni_env)->GetMethodID(jni_env, HashMap, "values", "()Ljava/util/Collection;");
	
	Iterator = (*jni_env)->FindClass(jni_env, "java/util/Iterator");
	Iterator_hasNext = (*jni_env)->GetMethodID(jni_env, Iterator, "hasNext", "()Z");
	Iterator_next = (*jni_env)->GetMethodID(jni_env, Iterator, "next", "()Ljava/lang/Object;");

	ActivityThread = (*jni_env)->FindClass(jni_env, "android/app/ActivityThread");
	ActivityThread__currentActivityThread = (*jni_env)->GetStaticMethodID(jni_env, ActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
	ActivityThread_getApplication = (*jni_env)->GetMethodID(jni_env, ActivityThread, "getApplication", "()Landroid/app/Application;");

	Context = (*jni_env)->FindClass(jni_env, "android/content/Context");
	Context__USB_SERVICE = (*jni_env)->GetStaticFieldID(jni_env, Context, "USB_SERVICE", "Ljava/lang/String;");
	Context_getPackageManager = (*jni_env)->GetMethodID(jni_env, Context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
	Context_getSystemService = (*jni_env)->GetMethodID(jni_env, Context, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");

	PackageManager = (*jni_env)->FindClass(jni_env, "android/content/pm/PackageManager");
	PackageManager__FEATURE_USB_HOST = (*jni_env)->GetStaticFieldID(jni_env, PackageManager, "FEATURE_USB_HOST", "Ljava/lang/String;");
	PackageManager_hasSystemFeature = (*jni_env)->GetMethodID(jni_env, PackageManager, "hasSystemFeature", "(Ljava/lang/String;)Z");

	UsbDevice = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbDevice");
	UsbDevice_getDeviceId = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getDeviceId", "()I");

	UsbManager = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbManager");
	UsbManager_getDeviceList = (*jni_env)->GetMethodID(jni_env, UsbManager, "getDeviceList", "()Ljava/util/HashMap;");

	return LIBUSB_SUCCESS;
}

static int android_jni_jnienv(JavaVM *javavm, JNIEnv **jni_env)
{
	return (*javavm)->AttachCurrentThread(javavm, jni_env, NULL) == JNI_OK ? LIBUSB_SUCCESS : LIBUSB_ERROR_OTHER;
}

static jobject android_jni_context(JNIEnv *jni_env)
{
	/* ActivityThread activityThread = ActivityThread.currentActivityThread(); */
	jobject activityThread = (*jni_env)->CallStaticObjectMethod(jni_env, ActivityThread, ActivityThread__currentActivityThread);
	/* return activityThread.getApplication(); */
	return (*jni_env)->CallObjectMethod(jni_env, activityThread, ActivityThread_getApplication);
}

int android_jni_detect_usbhost(JavaVM *javavm, int *has_usbhost)
{
	int r;
	JNIEnv *jni_env;
	jobject context, packageManager;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;

	context = android_jni_context(jni_env);

	/* PackageManager packageManager = context.getPackageManager(); */
	packageManager = (*jni_env)->CallObjectMethod(jni_env, context, Context_getPackageManager);

	/* has_usbhost = packageManager.hasSystemFeature(PackageManager.FEATURE_USB_HOST); */
	*has_usbhost = (*jni_env)->CallBooleanMethod(jni_env, packageManager, PackageManager_hasSystemFeature, (*jni_env)->GetStaticObjectField(jni_env, PackageManager, PackageManager__FEATURE_USB_HOST));

	return LIBUSB_SUCCESS;
}

struct android_jni_devices
{
	JavaVM *javavm;
	jobject iterator;
};

int android_jni_devices_alloc(JavaVM *javavm, struct android_jni_devices **devices)
{
	int r;
	JNIEnv *jni_env;
	jobject context, usbManager, deviceMap, deviceCollection, deviceIterator;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;

	context = android_jni_context(jni_env);
	
	/* UsbManager usbManager = context.getSystemService(Context.USB_SERVICE); */
	usbManager = (*jni_env)->CallObjectMethod(jni_env, context, Context_getSystemService, (*jni_env)->GetStaticObjectField(jni_env, Context, Context__USB_SERVICE));

	/* HashMap<String, UsbDevice> deviceMap = usbManager.getDeviceList(); */
	deviceMap = (*jni_env)->CallObjectMethod(jni_env, usbManager, UsbManager_getDeviceList);

	(*jni_env)->DeleteLocalRef(jni_env, usbManager);

	/* Collection<UsbDevice> deviceCollection = deviceMap.values(); */
	deviceCollection = (*jni_env)->CallObjectMethod(jni_env, deviceMap, HashMap_values);

	(*jni_env)->DeleteLocalRef(jni_env, deviceMap);

	/* Iterator<UsbDevice> deviceIterator = deviceCollection.iterator(); */
	deviceIterator = (*jni_env)->CallObjectMethod(jni_env, deviceCollection, Collection_iterator);

	(*jni_env)->DeleteLocalRef(jni_env, deviceCollection);

	*devices = malloc(sizeof(struct android_jni_devices));
	if (*devices == NULL)
		return LIBUSB_ERROR_NO_MEM;
	(*devices)->javavm = javavm;
	(*devices)->iterator = (*jni_env)->NewGlobalRef(jni_env, deviceIterator);

	(*jni_env)->DeleteLocalRef(jni_env, deviceIterator);

	return LIBUSB_SUCCESS;
}

int android_jni_devices_next(struct android_jni_devices *devices, jobject *device, uint8_t *busnum, uint8_t *devaddr)
{
	JNIEnv *jni_env;
	jobject deviceIterator = devices->iterator;
	jobject local_device;
	uint16_t deviceid;
	int r;

	r = android_jni_jnienv(devices->javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;

	/* if (!deviceIterator.hasNext()) */
	if (!(*jni_env)->CallBooleanMethod(jni_env, deviceIterator, Iterator_hasNext)) {

		return LIBUSB_ERROR_NOT_FOUND;
	}

	/* UsbDevice local_device = deviceIterator.next(); */
	local_device = (*jni_env)->CallObjectMethod(jni_env, deviceIterator, Iterator_next);

	/* int deviceid = device.getDeviceId(); */
	deviceid = (*jni_env)->CallIntMethod(jni_env, local_device, UsbDevice_getDeviceId);

	*device = (*jni_env)->NewGlobalRef(jni_env, local_device);
	(*jni_env)->DeleteLocalRef(jni_env, local_device);

	/* https://android.googlesource.com/platform/system/core/+/master/libusbhost/usbhost.c */
	*busnum = deviceid / 1000;
	*devaddr = deviceid % 1000;

	return LIBUSB_SUCCESS;
}

void android_jni_devices_free(struct android_jni_devices *devices)
{
	int r;
	JNIEnv *jni_env;

	r = android_jni_jnienv(devices->javavm, &jni_env);
	if (r == LIBUSB_SUCCESS)
		(*jni_env)->DeleteGlobalRef(jni_env, devices->iterator);

	free(devices);
}

void android_jni_globalunref(JavaVM *javavm, jobject *object)
{
	int r;
	JNIEnv *jni_env;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return;

	(*jni_env)->DeleteGlobalRef(jni_env, object);
}
