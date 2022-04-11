/*
 * dev.c
 * I/O system Device layer
 *      Author: ljp
 */
#include "queue.h"
#include "kern_api.h"
#include "ios_device.h"

#define DEVICE_MAGIC 0x44455653 //"DEVS"

// #define __device_init (dev->dev_ops.init)
#define __device_open (dev->dev_ops.open)
#define __device_close (dev->dev_ops.close)
#define __device_read (dev->dev_ops.read)
#define __device_write (dev->dev_ops.write)
#define __device_ioctl (dev->dev_ops.ioctl)

//link all devices
static TAILQ_HEAD(device_list_t, device_t) device_list;

static DEFINE_MUTEX(dev_sys_lock);
#define DEV_SYS_LOCK() mutex_take(&dev_sys_lock)
#define DEV_SYS_UNLOCK() mutex_give(&dev_sys_lock)
// #define DEV_SYS_LOCK() CPU_CRITICAL_ENTER()
// #define DEV_SYS_UNLOCK() CPU_CRITICAL_EXIT()

//creat(name)/open/close/read/write(fd) -> device_open -> dev_ops -> user device

//search the best match device. eg. "/gpio/PA.5" will match device "/gpio"
static device_t* device_find_best(const char *name) {
	device_t *dev;
    device_t *pBestDev = NULL;
    int maxLen = 0;
	int devlen;

	DEV_SYS_LOCK();
	TAILQ_FOREACH(dev, &device_list, dev_link)
	{
		//devlen = strlen(dev->dev_name);
		devlen = dev->dev_name_len;
		if (strncmp(dev->dev_name, name, devlen) == 0) {
			if(devlen > maxLen) {
				pBestDev = dev;
				maxLen = devlen;
			}
		}
	}

	DEV_SYS_UNLOCK();

	return pBestDev;
}

// static device_t* device_find_exact(const char *name) {
// 	device_t *dev;

// 	DEV_SYS_LOCK();
// 	TAILQ_FOREACH(dev, &device_list, dev_link)
// 	{
// 		if (strncmp(dev->dev_name, name, DEVICE_NAME_MAX) == 0) {
// 			DEV_SYS_UNLOCK();
// 			return dev;
// 		}
// 	}
// 	DEV_SYS_UNLOCK();

// 	return NULL;
// }

device_t* device_find(const char *name) {
	return device_find_best(name);
}

int device_register(device_t *dev, const char *name) {
	if (dev == NULL)
		return K_ERROR;

	if (device_find(name) != NULL)
		return K_ERROR;

	DEV_SYS_LOCK();
	TAILQ_INSERT_TAIL(&device_list, dev, dev_link);
	DEV_SYS_UNLOCK();

	strncpy(dev->dev_name, name, DEVICE_NAME_MAX);
	dev->dev_name_len = strlen(dev->dev_name);
	dev->ref_count = 0;
	dev->magic = DEVICE_MAGIC;

	return K_OK;
}


#define ENSURE_DEVICE_VALID(dev) 			\
do {										\
	ASSERT(dev != NULL);					\
	ASSERT(dev->magic == DEVICE_MAGIC);		\
} while(0)


#define ENSURE_DEVICE_OPENED(dev) 			\
do {										\
	ENSURE_DEVICE_VALID(dev);				\
	if (dev->ref_count <= 0)				\
		return K_ERROR;						\
} while(0)

int device_unregister(device_t *dev) {
	ENSURE_DEVICE_VALID(dev);

	DEV_SYS_LOCK();
	TAILQ_REMOVE(&device_list, dev, dev_link);
	DEV_SYS_UNLOCK();

	dev->magic = 0;
	dev->ref_count = 0;

	return K_OK;
}

static const char* translate_to_rel_path(device_t* dev, const char* src_path)
{
    ASSERT(strncmp(src_path, dev->dev_name, dev->dev_name_len) == 0);
    if (strlen(src_path) == dev->dev_name_len) {
        // special case when src_path matches the path prefix exactly
        return "/";
    }
    return src_path + dev->dev_name_len;
}

//return user data or error(-1)
int device_open(device_t *dev, const char *filename , int flags, int mode) {
	ENSURE_DEVICE_VALID(dev);

	int file_data = K_OK;

	//transate filename "/gpio/pa.5" to /pa.5
	const char *path_within_dev = translate_to_rel_path(dev, filename);

	if (__device_open != NULL) {
		file_data = __device_open(dev, path_within_dev, flags, mode);
	}

	/* save open count */
	if (file_data != -1) {
		dev->ref_count++;
	}

	return file_data;
}


int device_close(device_t *dev, void *file_data) {
	ENSURE_DEVICE_OPENED(dev);

	int result = K_OK;

	dev->ref_count--;

	if (dev->ref_count != 0)
		return K_OK;

	if (__device_close != NULL) {
		result = __device_close(dev, file_data);
	}

	return result;
}

int device_read(device_t *dev, void *file_data, char *buffer, int size) {
	ENSURE_DEVICE_OPENED(dev);

	/* call __device_read interface */
	if (__device_read != NULL) {
		return __device_read(dev, file_data, buffer, size);
	}

	return -1;
}

int device_write(device_t *dev, void *file_data, char *buffer, int size) {
	ENSURE_DEVICE_OPENED(dev);

	if (__device_write != NULL) {
		return __device_write(dev, file_data, buffer, size);
	}

	return -1;
}

int device_ioctl(device_t *dev, void *file_data, int cmd, void *arg) {
	ENSURE_DEVICE_OPENED(dev);

	if (__device_ioctl != NULL) {
		return __device_ioctl(dev, file_data, cmd, arg);
	}

	return -1;
}

void ios_device_init(void) {
	TAILQ_INIT(&device_list);
	mutex_init(&dev_sys_lock);
}

