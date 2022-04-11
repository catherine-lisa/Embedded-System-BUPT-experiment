#include "kern/kern_api.h"
#include "kern/ios_fd.h"
#include "pin.h"

//gpio device
//pin file: /gpio/port.no, eg. /gpio/PA.5

void pin_mode(const char *pin_name, int mode) {
    int fd = open(pin_name, 0 , 0);
    if(fd > 0) {
        ioctl(fd, IOCMD_PIN_SET_MODE, mode);
        close(fd);
    }
}

int pin_read(const char *pin_name) {
    int fd = open(pin_name, 0 , 0);
    if(fd > 0) {
        char value;
        int ret = read(fd, &value, 1);
        close(fd);
        if(ret < 0) {
            trace_printf("read pin %s failed\n", pin_name);
            return -1;
        } else {
            return value;
        }
    }
    return -1;
}

void pin_write(const char *pin_name, int value) {
    int fd = open(pin_name, 0 , 0);
    if(fd > 0) {
        char on = 1;
        if(value == 0) {
            on = 0;
        }
        write(fd, (char *)&on, 1);
        close(fd);
    }
}

