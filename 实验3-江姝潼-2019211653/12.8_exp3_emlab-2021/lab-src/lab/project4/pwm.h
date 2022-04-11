/*
 * device_pwm.h
 *
 *      Author: ljp
 */

#ifndef DEVICE_PWM_H_
#define DEVICE_PWM_H_

#include "kern/ios_device.h"

//pwm device name for device subsystem
#define PWM_DEV_PATH "/pwm"

//pwmcode = port << 4 | no
#define PWM_CODE(port, channel) (((((port) & 0xFu) << 4) | ((channel) & 0xFu)))
#define PWM_PORT(pwmcode) ((uint8_t)(((pwmcode) >> 4) & 0xFu))
#define PWM_CHANNEL(pwmcode) ((uint8_t)((pwmcode) & 0xFu))

//ioctl definition
#define IOCMD_PWM_SET       1
#define IOCMD_PWM_SET       1
#define IOCMD_PWM_ENABLE    2
#define IOCMD_PWM_DISABLE   3

struct pwm_device {
    device_t parent;
};

//pwm ioctl msg
struct pwm_ioctl_msg {
    // int pwm_num;
    // int channel;
    int period;
    int pulse;
};

//pwm api functions
int pwm_init(const char *pwmname);
int pwm_enable(const char *pwmname);
int pwm_disable(const char *pwmname);
int pwm_set(const char *pwmname, int period, int pulse);

//set driver callbacks and regist pwm device
void driver_pwm_init(void);




#endif /* DEVICE_PWM_H_ */
