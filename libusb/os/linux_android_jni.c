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

#include <stdio.h>
#include <string.h>

#include <jni.h>

static int android_jni_jnienv(JavaVM *javavm, JNIEnv **jni_env);
static jobject android_jni_context(JNIEnv *jni_env);
static int android_jni_gen_string(JNIEnv *jni_env, char **strings, size_t *strings_len, jstring str);
static int android_jni_gen_endpoint(JNIEnv *jni_env, jobject endpoint, uint8_t **descriptors, size_t *descriptors_len);
static int android_jni_gen_interface(JNIEnv *jni_env, jobject interface, uint8_t *num_ifaces, uint8_t **descriptors, size_t *descriptors_len, char **strings, size_t *strings_len);
static int android_jni_gen_configuration(JNIEnv *jni_env, jobject config, uint8_t **descriptors, size_t *descriptors_len, char **strings, size_t *strings_len);

static jclass Collection, HashMap, Iterator;
static jmethodID Collection_iterator;
static jmethodID HashMap_values;
static jmethodID Iterator_hasNext, Iterator_next;

static jclass ActivityThread, Build__VERSION, Context, Intent, PackageManager, PendingIntent, UsbConfiguration, UsbDevice, UsbDeviceConnection, UsbEndpoint, UsbInterface, UsbManager;
static jmethodID ActivityThread__currentActivityThread, ActivityThread_getApplication;
static jfieldID Build__VERSION__SDK_INT;
static const int Build__VERSION_CODES__P = 28;
static jfieldID Context__USB_SERVICE;
static jmethodID Context_getPackageManager, Context_getSystemService;
static jmethodID Intent_init;
static jfieldID PackageManager__FEATURE_USB_HOST;
static jmethodID PackageManager_hasSystemFeature;
static jmethodID PendingIntent__getBroadcast;
static jmethodID UsbConfiguration_getInterfaceCount, UsbConfiguration_getId, UsbConfiguration_getName, UsbConfiguration_isSelfPowered, UsbConfiguration_isRemoteWakeup, UsbConfiguration_getMaxPower, UsbConfiguration_getInterface;
static jmethodID UsbDevice_getDeviceId, UsbDevice_getConfigurationCount, UsbDevice_getVersion, UsbDevice_getDeviceClass, UsbDevice_getDeviceSubclass, UsbDevice_getDeviceProtocol, UsbDevice_getVendorId, UsbDevice_getProductId, UsbDevice_getManufacturerName, UsbDevice_getProductName, UsbDevice_getSerialNumber, UsbDevice_getConfiguration;
static jmethodID UsbDeviceConnection_close, UsbDeviceConnection_getFileDescriptor, UsbDeviceConnection_getRawDescriptors;
static jmethodID UsbEndpoint_getAddress, UsbEndpoint_getAttributes, UsbEndpoint_getMaxPacketSize, UsbEndpoint_getInterval;
static jmethodID UsbInterface_getEndpointCount, UsbInterface_getId, UsbInterface_getAlternateSetting, UsbInterface_getInterfaceClass, UsbInterface_getInterfaceSubclass, UsbInterface_getInterfaceProtocol, UsbInterface_getName, UsbInterface_getEndpoint;
static jmethodID UsbManager_getDeviceList, UsbManager_hasPermission, UsbManager_openDevice, UsbManager_requestPermission;

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

	Build__VERSION = (*jni_env)->FindClass(jni_env, "android/os/Build$VERSION");
	Build__VERSION__SDK_INT = (*jni_env)->GetStaticFieldID(jni_env, Build__VERSION, "SDK_INT", "I");

	Context = (*jni_env)->FindClass(jni_env, "android/content/Context");
	Context__USB_SERVICE = (*jni_env)->GetStaticFieldID(jni_env, Context, "USB_SERVICE", "Ljava/lang/String;");
	Context_getPackageManager = (*jni_env)->GetMethodID(jni_env, Context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
	Context_getSystemService = (*jni_env)->GetMethodID(jni_env, Context, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");

	Intent = (*jni_env)->FindClass(jni_env, "android/content/Intent");
	Intent_init = (*jni_env)->GetMethodID(jni_env, Intent, "<init>", "(Ljava/lang/String;)V");

	PackageManager = (*jni_env)->FindClass(jni_env, "android/content/pm/PackageManager");
	PackageManager__FEATURE_USB_HOST = (*jni_env)->GetStaticFieldID(jni_env, PackageManager, "FEATURE_USB_HOST", "Ljava/lang/String;");
	PackageManager_hasSystemFeature = (*jni_env)->GetMethodID(jni_env, PackageManager, "hasSystemFeature", "(Ljava/lang/String;)Z");

	PendingIntent = (*jni_env)->FindClass(jni_env, "android/app/PendingIntent");
	PendingIntent__getBroadcast = (*jni_env)->GetStaticMethodID(jni_env, PendingIntent, "getBroadcast", "(Landroid/content/Context;ILandroid/content/Intent;I)Landroid/app/PendingIntent;");

	UsbConfiguration = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbConfiguration");
	UsbConfiguration_getInterfaceCount = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "getInterfaceCount", "()I");
	UsbConfiguration_getId = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "getId", "()I");
	UsbConfiguration_getName = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "getName", "()Ljava/lang/String;");
	UsbConfiguration_isSelfPowered = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "isSelfPowered", "()Z");
	UsbConfiguration_isRemoteWakeup = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "isRemoteWakeup", "()Z");
	UsbConfiguration_getMaxPower = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "getMaxPower", "()I");
	UsbConfiguration_getInterface = (*jni_env)->GetMethodID(jni_env, UsbConfiguration, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");

	UsbDevice = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbDevice");
	UsbDevice_getDeviceId = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getDeviceId", "()I");
	UsbDevice_getConfigurationCount = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getConfigurationCount", "()I");
	UsbDevice_getVersion = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getVersion", "()Ljava/lang/String;");
	UsbDevice_getDeviceClass = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getDeviceClass", "()I");
	UsbDevice_getDeviceSubclass = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getDeviceSubclass", "()I");
	UsbDevice_getDeviceProtocol = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getDeviceProtocol", "()I");
	UsbDevice_getVendorId = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getVendorId", "()I");
	UsbDevice_getProductId = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getProductId", "()I");
	UsbDevice_getManufacturerName = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getManufacturerName", "()Ljava/lang/String;");
	UsbDevice_getProductName = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getProductName", "()Ljava/lang/String;");
	UsbDevice_getSerialNumber = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getSerialNumber", "()Ljava/lang/String;");
	UsbDevice_getConfiguration = (*jni_env)->GetMethodID(jni_env, UsbDevice, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");

	UsbDeviceConnection = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbDeviceConnection");
	UsbDeviceConnection_close = (*jni_env)->GetMethodID(jni_env, UsbDeviceConnection, "close", "()V");
	UsbDeviceConnection_getFileDescriptor = (*jni_env)->GetMethodID(jni_env, UsbDeviceConnection, "getFileDescriptor", "()I");
	UsbDeviceConnection_getRawDescriptors = (*jni_env)->GetMethodID(jni_env, UsbDeviceConnection, "getRawDescriptors", "()[B");

	UsbEndpoint = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbEndpoint");
	UsbEndpoint_getAddress = (*jni_env)->GetMethodID(jni_env, UsbEndpoint, "getAddress", "()I");
	UsbEndpoint_getAttributes = (*jni_env)->GetMethodID(jni_env, UsbEndpoint, "getAttributes", "()I");
	UsbEndpoint_getMaxPacketSize = (*jni_env)->GetMethodID(jni_env, UsbEndpoint, "getMaxPacketSize", "()I");
	UsbEndpoint_getInterval = (*jni_env)->GetMethodID(jni_env, UsbEndpoint, "getInterval", "()I");

	UsbInterface = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbInterface");
	UsbInterface_getEndpointCount = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getEndpointCount", "()I");
	UsbInterface_getId = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getId", "()I");
	UsbInterface_getAlternateSetting = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getAlternateSetting", "()I");
	UsbInterface_getInterfaceClass = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getInterfaceClass", "()I");
	UsbInterface_getInterfaceSubclass = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getInterfaceSubclass", "()I");
	UsbInterface_getInterfaceProtocol = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getInterfaceProtocol", "()I");
	UsbInterface_getName = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getName", "()Ljava/lang/String;");
	UsbInterface_getEndpoint = (*jni_env)->GetMethodID(jni_env, UsbInterface, "getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;");

	UsbManager = (*jni_env)->FindClass(jni_env, "android/hardware/usb/UsbManager");
	UsbManager_getDeviceList = (*jni_env)->GetMethodID(jni_env, UsbManager, "getDeviceList", "()Ljava/util/HashMap;");
	UsbManager_hasPermission = (*jni_env)->GetMethodID(jni_env, UsbManager, "hasPermission", "(Landroid/hardware/usb/UsbDevice;)Z");
	UsbManager_openDevice = (*jni_env)->GetMethodID(jni_env, UsbManager, "openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;");
	UsbManager_requestPermission = (*jni_env)->GetMethodID(jni_env, UsbManager, "requestPermission", "(Landroid/hardware/usb/UsbDevice;Landroid/app/PendingIntent;)V");

	return LIBUSB_SUCCESS;
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

int android_jni_gen_descriptors(JavaVM *javavm, jobject device, uint8_t **descriptors, size_t *descriptors_len, char **strings, size_t *strings_len)
{
	int r, num_configs, idx, version, tens, ones, tenths, hundredths, androidver;
	JNIEnv *jni_env;
	jobject config;
	jstring jversion;
	const char *sversion;
	struct usbi_string_descriptor string_desc;
	struct usbi_device_descriptor desc;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;

	/* int num_configs = device.getConfigurationCount(); */
	num_configs = (*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getConfigurationCount);

	*strings_len = 2;
	*strings = malloc(*strings_len);
	if (!*strings)
		return LIBUSB_ERROR_NO_MEM;
	
	*descriptors_len = LIBUSB_DT_DEVICE_SIZE;
	*descriptors = malloc(*descriptors_len);
	if (!*descriptors)
		return LIBUSB_ERROR_NO_MEM;

	string_desc = (struct usbi_string_descriptor){
		.bLength = *strings_len,
		.bDescriptorType = LIBUSB_DT_STRING,
	};
	*(struct usbi_string_descriptor *)*strings = string_desc;

	/* parse binary coded decimal version */
	/* String jversion = device.getVersion(); */
	jversion = (*jni_env)->CallObjectMethod(jni_env, device, UsbDevice_getVersion);

	sversion = (*jni_env)->GetStringUTFChars(jni_env, jversion, NULL);
	sscanf(sversion, "%d.%d", &ones, &hundredths);
	(*jni_env)->ReleaseStringUTFChars(jni_env, jversion, sversion);

	/* Android usb version bug was fixed in commit 608ec66d62647f60c3988922fead33fd7e07755e in Pie */
	/* int androidver = Build.VERSION.SDK_INT; */
	androidver = (*jni_env)->GetStaticIntField(jni_env, Build__VERSION, Build__VERSION__SDK_INT);
	if (androidver < Build__VERSION_CODES__P) {
		/* undo pre-pie bug */
		tenths = hundredths / 16;
		hundredths = hundredths % 16;
	} else {
		/* bug has been fixed */
		tenths = hundredths / 10;
		hundredths = hundredths % 10;
	}
	tens = ones / 10;
	ones = ones % 10;

	version = hundredths | (tenths << 4) | (ones << 8) | (tens << 12);

	desc = (struct usbi_device_descriptor){
		.bLength = LIBUSB_DT_DEVICE_SIZE,
		.bDescriptorType = LIBUSB_DT_DEVICE,
		.bcdUSB = libusb_cpu_to_le16(version),
		.bDeviceClass = (*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getDeviceClass),
		.bDeviceSubClass = (*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getDeviceSubclass),
		.bDeviceProtocol = (*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getDeviceProtocol),
		.bMaxPacketSize0 = 8,
		.idVendor = libusb_cpu_to_le16((*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getVendorId)),
		.idProduct = libusb_cpu_to_le16((*jni_env)->CallIntMethod(jni_env, device, UsbDevice_getProductId)),
		.bcdDevice = 0xFFFF,
		.iManufacturer = android_jni_gen_string(jni_env, strings, strings_len,
			(*jni_env)->CallObjectMethod(jni_env, device, UsbDevice_getManufacturerName)),
		.iProduct = android_jni_gen_string(jni_env, strings, strings_len,
			(*jni_env)->CallObjectMethod(jni_env, device, UsbDevice_getProductName)),
		.iSerialNumber = android_jni_gen_string(jni_env, strings, strings_len,
			(*jni_env)->CallObjectMethod(jni_env, device, UsbDevice_getSerialNumber)),
		.bNumConfigurations = num_configs,
	};
	if (desc.iManufacturer < 0) {
		return desc.iManufacturer;
	}
	if (desc.iProduct < 0) {
		return desc.iProduct;
	}
	if (desc.iSerialNumber < 0) {
		return desc.iSerialNumber;
	}
	*(struct usbi_device_descriptor *)*descriptors = desc;

	for (idx = 0; idx < num_configs; idx ++) {
		/* UsbConfiguration config = device.getConfiguration(i); */
		config = (*jni_env)->CallObjectMethod(jni_env, device, UsbDevice_getConfiguration, idx);

		r = android_jni_gen_configuration(jni_env, config, descriptors, descriptors_len, strings, strings_len);

		(*jni_env)->DeleteLocalRef(jni_env, config);

		if (r != LIBUSB_SUCCESS)
			return r;
	}

	return LIBUSB_SUCCESS;
}

int android_jni_connect(JavaVM *javavm, jobject device, jobject *connection, int *fd, uint8_t **descriptors, size_t *descriptors_len)
{
	int permission, r;
	JNIEnv *jni_env;
	jobject context, usbManager, intent, permissionIntent, local_connection;
	jbyteArray local_descriptors;
	jbyte * local_descriptors_ptr;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;
	context = android_jni_context(jni_env);
	
	/* UsbManager usbManager = context.getSystemService(Context.USB_SERVICE); */
	usbManager = (*jni_env)->CallObjectMethod(jni_env, context, Context_getSystemService, (*jni_env)->GetStaticObjectField(jni_env, Context, Context__USB_SERVICE));

	/* boolean permission = usbManager.hasPermission(device); */
	permission = (*jni_env)->CallBooleanMethod(jni_env, usbManager, UsbManager_hasPermission, device);

	if (!permission) {

		/* Intent intent = new Intent("libusb.android.USB_PERMISSION"); */
		intent = (*jni_env)->NewObject(jni_env, Intent, Intent_init, (*jni_env)->NewStringUTF(jni_env, "libusb.android.USB_PERMISSION"));

		/* PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, intent, 0); */
		permissionIntent = (*jni_env)->CallStaticObjectMethod(jni_env, PendingIntent, PendingIntent__getBroadcast, context, 0, intent, 0);

		(*jni_env)->DeleteLocalRef(jni_env, intent);

		/* usbManager.requestPermission(device, permissionIntent); */
		(*jni_env)->CallVoidMethod(jni_env, usbManager, UsbManager_requestPermission, device, permissionIntent);

		(*jni_env)->DeleteLocalRef(jni_env, permissionIntent);
		(*jni_env)->DeleteLocalRef(jni_env, usbManager);

		return LIBUSB_ERROR_ACCESS;
	}

	/* UsbDeviceConnection local_connection = usbManager.openDevice(device); */
	local_connection = (*jni_env)->CallObjectMethod(jni_env, usbManager, UsbManager_openDevice, device);

	(*jni_env)->DeleteLocalRef(jni_env, usbManager);

	/* fd output */
	/* fd = local_connection.getFileDescriptor(); */
	*fd = (*jni_env)->CallIntMethod(jni_env, local_connection, UsbDeviceConnection_getFileDescriptor);

	if (*fd == -1) {
		(*jni_env)->DeleteLocalRef(jni_env, local_connection);
		return LIBUSB_ERROR_IO;
	}
	
	/* byte[] local_descriptors = local_connection.getRawDescriptors(); */
	local_descriptors = (*jni_env)->CallObjectMethod(jni_env, local_connection, UsbDeviceConnection_getRawDescriptors);

	/* descriptors buffer output */
	*descriptors_len = (*jni_env)->GetArrayLength(jni_env, local_descriptors);
	*descriptors = malloc(*descriptors_len);
	if (!*descriptors) {
		(*jni_env)->DeleteLocalRef(jni_env, local_descriptors);
		(*jni_env)->DeleteLocalRef(jni_env, local_connection);
		return LIBUSB_ERROR_NO_MEM;
	}

	/* jni buffer copy */
	local_descriptors_ptr = (*jni_env)->GetPrimitiveArrayCritical(jni_env, local_descriptors, NULL);
	memcpy(*descriptors, local_descriptors_ptr, *descriptors_len);
	(*jni_env)->ReleasePrimitiveArrayCritical(jni_env, local_descriptors, local_descriptors_ptr, JNI_ABORT);

	/* connection jobject output */
	*connection = (*jni_env)->NewGlobalRef(jni_env, local_connection);

	(*jni_env)->DeleteLocalRef(jni_env, local_descriptors);
	(*jni_env)->DeleteLocalRef(jni_env, local_connection);

	return LIBUSB_SUCCESS;
}

int android_jni_disconnect(JavaVM *javavm, jobject connection)
{
	int r;
	JNIEnv *jni_env;

	r = android_jni_jnienv(javavm, &jni_env);
	if (r != LIBUSB_SUCCESS)
		return r;

	/* connection.close(); */
	(*jni_env)->CallVoidMethod(jni_env, connection, UsbDeviceConnection_close);

	return LIBUSB_SUCCESS;
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

static int android_jni_gen_configuration(JNIEnv *jni_env, jobject config, uint8_t **descriptors, size_t *descriptors_len, char **strings, size_t *strings_len)
{
	int r;
	size_t num_ifaces, offset, idx;
	struct usbi_configuration_descriptor desc;

	/* int num_ifaces = config.getInterfaceCount(); */
	num_ifaces = (*jni_env)->CallIntMethod(jni_env, config, UsbConfiguration_getInterfaceCount);

	offset = *descriptors_len;
	*descriptors_len = offset + LIBUSB_DT_CONFIG_SIZE;
	*descriptors = usbi_reallocf(*descriptors, *descriptors_len);
	if (!*descriptors)
		return LIBUSB_ERROR_NO_MEM;

	desc = (struct usbi_configuration_descriptor){
		.bLength = LIBUSB_DT_CONFIG_SIZE,
		.bDescriptorType = LIBUSB_DT_CONFIG,
		.wTotalLength = 0, /* assigned below */
		.bNumInterfaces = 0, /* assigned within interfaces */
		.bConfigurationValue = (*jni_env)->CallIntMethod(jni_env, config, UsbConfiguration_getId),
		.iConfiguration = android_jni_gen_string(jni_env, strings, strings_len,
			(*jni_env)->CallObjectMethod(jni_env, config, UsbConfiguration_getName)),
		.bmAttributes = 0x80 |
			((*jni_env)->CallBooleanMethod(jni_env, config, UsbConfiguration_isSelfPowered)
			 ? 0x40 : 0) |
			((*jni_env)->CallBooleanMethod(jni_env, config, UsbConfiguration_isRemoteWakeup)
			 ? 0x20 : 0),
		.bMaxPower = (*jni_env)->CallIntMethod(jni_env, config, UsbConfiguration_getMaxPower) / 2
	};
	if (desc.iConfiguration < 0) {
		return desc.iConfiguration;
	}

	for (idx = 0; idx < num_ifaces; ++ idx) {
		/* UsbInterface interface = config.getInterface(idx); */
		jobject interface = (*jni_env)->CallObjectMethod(jni_env, config, UsbConfiguration_getInterface, idx);

		r = android_jni_gen_interface(jni_env, interface, &desc.bNumInterfaces, descriptors, descriptors_len, strings, strings_len);

		(*jni_env)->DeleteLocalRef(jni_env, interface);

		if (r != LIBUSB_SUCCESS)
			return r;
	}
	
	desc.wTotalLength = libusb_cpu_to_le16(*descriptors_len - offset);
	*(struct usbi_configuration_descriptor *)(*descriptors + offset) = desc;

	return LIBUSB_SUCCESS;
}

int android_jni_gen_interface(JNIEnv *jni_env, jobject interface, uint8_t *num_ifaces, uint8_t **descriptors, size_t *descriptors_len, char **strings, size_t *strings_len)
{
	int r;
	size_t num_epoints, offset, idx;
	struct usbi_interface_descriptor desc;

	/* int num_epoints = interface.getEndpointCount(); */
	num_epoints = (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getEndpointCount);

	offset = *descriptors_len;
	*descriptors_len = offset + LIBUSB_DT_INTERFACE_SIZE;
	*descriptors = usbi_reallocf(*descriptors, *descriptors_len);
	if (!*descriptors)
		return LIBUSB_ERROR_NO_MEM;

	desc = (struct usbi_interface_descriptor){
		.bLength = LIBUSB_DT_INTERFACE_SIZE,
		.bDescriptorType = LIBUSB_DT_INTERFACE,
		.bInterfaceNumber =  (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getId),
		.bAlternateSetting =  (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getAlternateSetting),
		.bNumEndpoints = num_epoints,
		.bInterfaceClass =  (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getInterfaceClass),
		.bInterfaceSubClass =  (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getInterfaceSubclass),
		.bInterfaceProtocol =  (*jni_env)->CallIntMethod(jni_env, interface, UsbInterface_getInterfaceProtocol),
		.iInterface = android_jni_gen_string(jni_env, strings, strings_len,
			(*jni_env)->CallObjectMethod(jni_env, interface, UsbInterface_getName))
	};
	if (desc.iInterface < 0) {
		return desc.iInterface;
	}
	*(struct usbi_interface_descriptor *)(*descriptors + offset) = desc;

	if (desc.bInterfaceNumber >= *num_ifaces)
		*num_ifaces = desc.bInterfaceNumber + 1;

	for (idx = 0; idx < num_epoints; idx ++) {
		/* UsbEndpoint endpoint = interface.getEndpoint(idx); */
		jobject endpoint = (*jni_env)->CallObjectMethod(jni_env, interface, UsbInterface_getEndpoint, idx);

		r = android_jni_gen_endpoint(jni_env, endpoint, descriptors, descriptors_len);

		(*jni_env)->DeleteLocalRef(jni_env, endpoint);

		if (r != LIBUSB_SUCCESS)
			return r;
	}
	return LIBUSB_SUCCESS;
}

static int android_jni_gen_endpoint(JNIEnv *jni_env, jobject endpoint, uint8_t **descriptors, size_t *descriptors_len)
{
	size_t offset;
	struct usbi_endpoint_descriptor desc;

	offset = *descriptors_len;
	*descriptors_len = offset + LIBUSB_DT_ENDPOINT_SIZE;
	*descriptors = usbi_reallocf(*descriptors, *descriptors_len);
	if (!*descriptors)
		return LIBUSB_ERROR_NO_MEM;

	desc = (struct usbi_endpoint_descriptor){
		.bLength = LIBUSB_DT_ENDPOINT_SIZE,
		.bDescriptorType = LIBUSB_DT_ENDPOINT,
		.bEndpointAddress = (*jni_env)->CallIntMethod(jni_env, endpoint, UsbEndpoint_getAddress),
		.bmAttributes = (*jni_env)->CallIntMethod(jni_env, endpoint, UsbEndpoint_getAttributes),
		.wMaxPacketSize = libusb_cpu_to_le16((*jni_env)->CallIntMethod(jni_env, endpoint, UsbEndpoint_getMaxPacketSize)),
		.bInterval = (*jni_env)->CallIntMethod(jni_env, endpoint, UsbEndpoint_getInterval)
	};
	*(struct usbi_endpoint_descriptor *)(*descriptors + offset) = desc;
	
	return LIBUSB_SUCCESS;
	
}

static int android_jni_gen_string(JNIEnv *jni_env, char **strings, size_t *strings_len, jstring str)
{
	size_t str_len, offset, idx;
	const uint16_t *str_ptr;
	struct usbi_string_descriptor *desc;

	if ((*jni_env)->IsSameObject(jni_env, str, NULL) ) {
		return 0;
	}

	str_ptr = (*jni_env)->GetStringChars(jni_env, str, NULL);
	str_len = (*jni_env)->GetStringLength(jni_env, str) * sizeof(str_ptr[0]);

	for (offset = 0, idx = 0; offset < *strings_len; offset += desc->bLength, ++ idx) {
		desc = (struct usbi_string_descriptor *)(*strings + offset);
		if (desc->bLength - 2 == str_len) {
			if (memcmp(desc->wData, str_ptr, str_len) == 0) {
				break;
			}
		}
	}

	if (offset == *strings_len) {
		*strings_len += 2 + str_len;
		*strings = usbi_reallocf(*strings, *strings_len);
		if (!*strings)
			return LIBUSB_ERROR_NO_MEM;
		
		desc = (struct usbi_string_descriptor *)(*strings + offset);
		*desc = (struct usbi_string_descriptor){
			.bLength = 2 + str_len,
			.bDescriptorType = LIBUSB_DT_STRING,
		};
		memcpy(desc->wData, str_ptr, str_len);
	}

	(*jni_env)->ReleaseStringChars(jni_env, str, str_ptr);

	return idx;
}

static jobject android_jni_context(JNIEnv *jni_env)
{
	/* ActivityThread activityThread = ActivityThread.currentActivityThread(); */
	jobject activityThread = (*jni_env)->CallStaticObjectMethod(jni_env, ActivityThread, ActivityThread__currentActivityThread);

	/* return activityThread.getApplication(); */
	return (*jni_env)->CallObjectMethod(jni_env, activityThread, ActivityThread_getApplication);
}

static int android_jni_jnienv(JavaVM *javavm, JNIEnv **jni_env)
{
	return (*javavm)->AttachCurrentThread(javavm, jni_env, NULL) == JNI_OK ? LIBUSB_SUCCESS : LIBUSB_ERROR_OTHER;
}
