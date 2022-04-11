/*
 * ios_fd.h
 *
 *      Author: ljp
 */

#ifndef IOS_FD_H_
#define IOS_FD_H_

struct device_t;

void ios_fd_init(void);

int stdio_get(int stdFd);
void stdio_set(int stdFd, int newFd);

struct device_t *ios_fd_get_device(int fd);
void* ios_fd_data(int fd);
void ios_fd_set(int fd, struct device_t *dev, char *name, void* file_data);
int ios_fd_new(struct device_t *dev, char *name, void* file_data);
void ios_fd_free(int fd);

int ios_fd_open(const char *file, int flags, int mode);
int ios_fd_close(int fd);
int ios_fd_read(int fd, char *buf, int len);
int ios_fd_write(int fd, char *buf, int len);
int ios_fd_ioctl(int fd, int cmd, int arg);

static inline int open(const char *file, int flags, int mode) {
    return ios_fd_open(file, flags, mode);
}
static inline int close(int fd) {
    return ios_fd_close(fd);
}
static inline int read(int fd, char *buf, int len) {
    return ios_fd_read(fd, buf, len);
}
static inline int write(int fd, char *buf, int len) {
    return ios_fd_write(fd, buf, len);
}
static inline int ioctl(int fd, int cmd, int arg) {
    return ios_fd_ioctl(fd, cmd, arg);
}

#endif /* IOS_FD_H_ */
