/*
 * I/O System File Descriptor layer
 */
#include "kern_api.h"
#include "ios_device.h"
#include "ios_fd.h"

#define CONFIG_MAX_FILES 16

#define FD_MAGIC 0xfdfdfdfd
#define STD_FIX(fd) ((fd) + 3)
#define STD_UNFIX(fd) ((fd)-3)

typedef struct fd_t {
	device_t *dev; /* device header for this file */
	char *name; /* actual file name */
	int ref_count;
	unsigned int magic;

	void *file_data; /* file private info */
} fd_t;

static DEFINE_MUTEX(ios_lock);
#define IOS_LOCK() mutex_take(&ios_lock)
#define IOS_UNLOCK() mutex_give(&ios_lock)

#define USER_FD_START 3
#define INVALID_FD (-1)
#define STD_VALID(fd) (((fd) >= 0) && ((fd) < 3))

static int maxFiles = CONFIG_MAX_FILES;
static fd_t fd_table[CONFIG_MAX_FILES];
static int std_fd_redirect[3] = { INVALID_FD, INVALID_FD, INVALID_FD };

void ios_fd_init(void) {
	maxFiles = NELEM(fd_table);
	memset(fd_table, 0, sizeof(fd_table));
	std_fd_redirect[0] = std_fd_redirect[1] = std_fd_redirect[2] = INVALID_FD;
	mutex_init(&ios_lock);
}

//stdio get/set

int stdio_get(int stdFd) {
	return (STD_VALID(stdFd) ? std_fd_redirect[stdFd] : -1);
}

void stdio_set(int stdFd, int newFd) {
	if (STD_VALID(stdFd)) {
		std_fd_redirect[stdFd] = newFd;
	}
}

//fd new/free
#define FD_INUSE(pFdEntry) (pFdEntry && pFdEntry->ref_count > 0)

static fd_t* fd_entry(int fd) {
	int xfd = STD_VALID(fd) ? std_fd_redirect[fd] : fd;
	if (xfd >= USER_FD_START && xfd <= maxFiles) {
		return &fd_table[xfd];
	}
	return NULL;
}

device_t*  ios_fd_get_device(int fd) {
	fd_t *pFdEntry = fd_entry(fd);
	if (FD_INUSE(pFdEntry)) {
		return (pFdEntry->dev);
	} else {
		return (NULL);
	}
}

void* ios_fd_data(int fd) {
	fd_t *pFdEntry = fd_entry(fd);
	if (FD_INUSE(pFdEntry)) {
		return (pFdEntry->file_data);
	} else {
		return (NULL);
	}
}

void ios_fd_set(int fd, device_t *dev, char *name, void* file_data) {
	ASSERT(fd < maxFiles);
	fd_t *pFdEntry = fd_entry(fd);

	/* if no name specified, set it NULL;
	 * if name is same as device name, make fd name point to device name;
	 * otherwise point to the given name
	 */
	if (name == NULL) {
		pFdEntry->name = NULL;
	} else if (strcmp(name, dev->dev_name) == 0) {
		pFdEntry->name = dev->dev_name;
	} else {
		pFdEntry->name = name;
	}

	pFdEntry->dev = dev;
	pFdEntry->file_data = file_data;
}

int ios_fd_new(device_t *dev, char *name, void* file_data) {
	int fd;
	fd_t *pFdEntry;

	IOS_LOCK();
	for (fd = USER_FD_START; fd < maxFiles; fd++) {
		pFdEntry = &fd_table[fd];
		if (pFdEntry->ref_count == 0) {
			pFdEntry->magic = FD_MAGIC;
			pFdEntry->ref_count = 1; /* reserve this entry */
			pFdEntry->name = NULL;
			break;
		}
	}
	IOS_UNLOCK();

	ios_fd_set(fd, dev, name, file_data);

	return fd;
}

void ios_fd_free(int fd) {
	ASSERT(fd >= 0 && fd < maxFiles);

	fd_t *pFdEntry = fd_entry(fd);

	if ((pFdEntry) != NULL) {
		ASSERT(pFdEntry->magic == FD_MAGIC);
		ASSERT(pFdEntry->ref_count > 0);

		pFdEntry->ref_count--;
		if (pFdEntry->ref_count == 0) {
			pFdEntry->magic = 0;
			pFdEntry->name = NULL;
		}
	}
}

//open/close/read/write/ioctl

int ios_fd_open(const char *file, int flags, int mode) {
	int fd, file_data;
	device_t *dev;

	dev = device_find(file);
	if (!dev) {
		return -1;
	}

	fd = ios_fd_new(dev, NULL, 0);
	if (fd == -1) {
		return -1;
	}

	file_data = device_open(dev, file, flags, mode);
	if (file_data == -1) {
		ios_fd_free(fd);
		return -1;
	}

	//save dev point + file name + result(file data) to fd table
	ios_fd_set(fd, dev, (char*)file, (void*)file_data);

	return fd;
}

#define ENSURE_GET_DEV_FDENTRY(dev, pFdEntry, fd) 			\
	device_t *dev;					\
	fd_t *pFdEntry = fd_entry(fd);	\
	if(!pFdEntry) {					\
		return -1;					\
	}								\
	dev =  ios_fd_get_device(fd);	\
	if (dev == NULL) {				\
		return -1;					\
	}

int ios_fd_close(int fd) {
	int result;

	ENSURE_GET_DEV_FDENTRY(dev, pFdEntry, fd) ;

	result = device_close(dev, pFdEntry->file_data);

	ios_fd_free(fd);

	return result;
}

int ios_fd_read(int fd, char *buf, int len) {
	ENSURE_GET_DEV_FDENTRY(dev, pFdEntry, fd) ;

	return device_read(dev, pFdEntry->file_data, buf, len);
}

int ios_fd_write(int fd, char *buf, int len) {
	ENSURE_GET_DEV_FDENTRY(dev, pFdEntry, fd) ;

	return device_write(dev, pFdEntry->file_data, buf, len);
}

int ios_fd_ioctl(int fd, int cmd, int arg) {
	ENSURE_GET_DEV_FDENTRY(dev, pFdEntry, fd) ;

	return device_ioctl(dev, pFdEntry->file_data, cmd, (void*) arg);
}
