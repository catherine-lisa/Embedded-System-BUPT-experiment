/*
 * device_pwm.c
 *
 *      Author: ljp
 */
#include "kern/ios_fd.h"
#include "pwm.h"
//pwm file: pwm/num.channel, eg. pwm/10.1
int pwm_init(const char *pwmname) {
	(void)pwmname;
    return 0;
}

int pwm_enable(const char *pwmname) {
    int fd = open(pwmname, 0, 0);
    char on = 1;
    if(fd > 0) {
        // ioctl(fd, IOCMD_PWM_ENABLE, 0);
        write(fd, (char *)&on, 1);
        close(fd);
        return 0;
    }
    return -1;
}

int pwm_disable(const char *pwmname) {
    int fd = open(pwmname, 0, 0);
    char on = 0;
    if(fd > 0) {
        // ioctl(fd, IOCMD_PWM_DISABLE, 0);
        write(fd, (char *)&on, 1);
        close(fd);
        return 0;
    }
    return -1;
}

//period and pulse: ns
int pwm_set(const char *pwmname, int period, int pulse) {
    int fd = open(pwmname, 0, 0);
    if(fd > 0) {
        struct pwm_ioctl_msg msg;
        msg.period = period;
        msg.pulse = pulse;
        ioctl(fd, IOCMD_PWM_SET, (int)&msg);
        close(fd);
        return 0;
    }
    return -1;
}
