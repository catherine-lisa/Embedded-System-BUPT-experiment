/*
 * device_pin.h
 *
 *      Author: ljp
 */

#ifndef DEVICE_PIN_H_
#define DEVICE_PIN_H_

#include "kern/ios_device.h"

//pin device name for device subsystem
#define PIN_DEV_PATH "/pin"

#define PIN_LOW                 0x00
#define PIN_HIGH                0x01

//pincode = port << 4 | no
#define PIN_CODE(port, no) (((((port) & 0xFu) << 4) | ((no) & 0xFu)))
#define PIN_PORT(pincode) ((uint8_t)(((pincode) >> 4) & 0xFu))
#define PIN_NO(pincode) ((uint8_t)((pincode) & 0xFu))

//ioctl definition
#define IOCMD_PIN_SET_MODE  1
// #define IOCMD_PIN_READ      2
// #define IOCMD_PIN_WRITE     3

//pin mode arg code
#define PIN_MODE_OUTPUT         0x00
#define PIN_MODE_INPUT          0x01
#define PIN_MODE_INPUT_PULLUP   0x02
#define PIN_MODE_INPUT_PULLDOWN 0x03
#define PIN_MODE_OUTPUT_OD      0x04

//pin ioctl msg
struct pin_ioctl_msg {
    // int pincode;
    int value;
};

//pin device struct
struct pin_device {
    device_t parent;
};

//pin api functions
void pin_mode(const char *pin_name, int mode);
void pin_write(const char *pin_name, int value);
int  pin_read(const char *pin_name);

//set driver callbacks and regist pin device
void driver_pin_init(void);

#endif /* DEVICE_PIN_H_ */
