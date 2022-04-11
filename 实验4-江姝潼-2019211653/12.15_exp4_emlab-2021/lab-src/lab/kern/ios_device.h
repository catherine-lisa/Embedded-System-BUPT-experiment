/*
 * ios_device.h
 *
 *      Author: ljp
 */

#ifndef IOS_DEVICE_H_
#define IOS_DEVICE_H_

#include "queue.h"

#define DEVICE_NAME_MAX 16

// #define DEVICE_FLAG_RDONLY 		0x001 	/* read only */
// #define DEVICE_FLAG_WRONLY 		0x002 	/* write only */
// #define DEVICE_FLAG_RDWR 		0x003	 /* read and write */

// #define DEVICE_FLAG_OPEN 		0x020	/* device is opened */
// #define DEVICE_FLAG_EXCLUSIVE 	0x100 	/* device should exclusive access */

struct device_t;

typedef struct device_ops_t
{
//	int (*init) (struct device_t *dev);
	int (*open) (struct device_t *dev, const char *filename, int flags, int mode);
	int (*close)(struct device_t *dev, void *file_data);
	int (*read) (struct device_t *dev, void *file_data, char *buffer, int size);
	int (*write)(struct device_t *dev, void *file_data, char *buffer, int size);
	int (*ioctl)(struct device_t *dev, void *file_data, int cmd, void *args);
} device_ops_t;

typedef int (*device_callback_t)(struct device_t *dev, void *buffer, int size);

typedef struct device_t
{
	TAILQ_ENTRY(device_t) dev_link; /* device link */
	char dev_name[DEVICE_NAME_MAX];
	unsigned int dev_name_len;

	int flag;
	int ref_count;
	//int inuse;
	unsigned int magic;

	//user provided
	device_ops_t dev_ops; /* device driver operations */

	void *dev_data;
} device_t;

#define CONTAINER_OF(ptr, type, member) \
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

//sub device's first field must be device_t dev_base;
#define DEVICE_CAST(subdev) ((device_t *)(subdev))

void ios_device_init(void);

device_t *device_find(const char *name);

int device_register(device_t *dev, const char *name);
int device_unregister(device_t *dev);

//open file "filenanme" in "dev", return file_data or -1
int device_open (device_t *dev, const char *filename, int flags, int mode);
int device_close(device_t *dev, void *file_data);
int device_read (device_t *dev, void *file_data, char *buffer, int size);
int device_write(device_t *dev, void *file_data, char *buffer, int size);
int device_ioctl(device_t *dev, void *file_data, int cmd, void *arg);

#endif /* IOS_DEVICE_H_ */
