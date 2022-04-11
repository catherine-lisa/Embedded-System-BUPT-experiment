#include <string.h>
#include "stm32f4xx_hal.h"
#include "pin.h"

#define PIN_TYPEDEF(pincode) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400u * PIN_PORT(pincode))))
#define GPIO_PIN_BIT(pincode) ((uint16_t)(1u << PIN_NO(pincode)))
#define RCC_AHB1ENR_GPIOxEN(pincode)   ((uint32_t)(1 << PIN_NO(pincode)))
#define __HAL_RCC_GPIOx_CLK_ENABLE(pincode)   do { \
                                        __IO uint32_t tmpreg = 0x00U; \
                                        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOxEN(pincode));\
                                        UNUSED(tmpreg); \
                                          } while(0)

static struct pin_device _pin_dev;

// /PF.10 -> PF.10
static const char * get_file_name(const char *pathname) {
    int i;
    const char *filename = pathname;
    //skip path name
    for (i = 0; pathname[i] != 0; i++) {
        if(pathname[i] == '/') {
            filename = pathname + i + 1;
        }
    }
    return filename;
}

//name "PF.10" -> port_num = 10, pin_num = 5
static int stm32_pin_get(const char *name)
{
    int pincode = 0;
    int port_num, pin_num = 0;
    int i, name_len;
    name = get_file_name(name);
    name_len = strlen(name);
    
    if ((name_len < 4) || (name_len > 5)) {
        return -1;
    }
    if ((name[0] != 'P') || (name[2] != '.')) {
        return -1;
    }

    if ((name[1] >= 'A') && (name[1] <= 'Z')) {
        port_num = (int)(name[1] - 'A');
    }
    else {
        return -1;
    }

    for (i = 3; i < name_len; i++) {
        pin_num *= 10;
        pin_num += name[i] - '0';
    }

    pincode = PIN_CODE(port_num, pin_num);

    return pincode;
}

//dev name as "/pin", filename as "PC.13" -> 0xCD
static int stm32_pin_open(struct device_t *dev, const char *filename, int flags, int mode) {
    (void)dev;
    (void)flags;
    (void)mode;
    //get pincode
    int pincode = stm32_pin_get(filename);
    if(pincode == -1) {
        return -1;
    }

    //enable port
    __HAL_RCC_GPIOx_CLK_ENABLE(pincode);

    return pincode;
}

int stm32_pin_close(struct device_t *dev, void* file_data) {
    (void)dev;
    (void)file_data;
    //disable port?
    return 0;
}

static int stm32_pin_read(struct device_t *dev, void* file_data, char *buffer, int size) {
    (void)dev;
    ASSERT(size == 1);
    int pincode = (int)file_data;
    GPIO_TypeDef * gpio_typ = PIN_TYPEDEF(pincode);
    uint16_t gpio_pin_bit = GPIO_PIN_BIT(pincode);
    buffer[0]  = HAL_GPIO_ReadPin(gpio_typ, gpio_pin_bit);
    return 1;
}
static int stm32_pin_write(struct device_t *dev, void* file_data, char *buffer, int size) {
    (void)dev;
    ASSERT(size == 1);
    int pincode = (int)file_data;
    GPIO_TypeDef * gpio_typ = PIN_TYPEDEF(pincode);
    uint16_t gpio_pin_bit = GPIO_PIN_BIT(pincode);
	HAL_GPIO_WritePin(gpio_typ, gpio_pin_bit, buffer[0] == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    return 1;
}

static int stm32_pin_ioctl(struct device_t *dev, void* file_data, int cmd, void *args) {
    (void)dev;
    int pincode = (int)file_data;
    GPIO_TypeDef * gpio_typ = PIN_TYPEDEF(pincode);
    // uint16_t gpio_pin_bit = GPIO_PIN_BIT(pincode);

    __HAL_RCC_GPIOx_CLK_ENABLE(pincode);

    switch (cmd)
    {
    case IOCMD_PIN_SET_MODE:
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        int mode = (int)args;
        GPIO_InitStruct.Pin = GPIO_PIN_BIT(pincode);
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        if (mode == PIN_MODE_OUTPUT)
        {
            /* output setting */
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
        }
        else if (mode == PIN_MODE_INPUT)
        {
            /* input setting: not pull. */
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
        }
        else if (mode == PIN_MODE_INPUT_PULLUP)
        {
            /* input setting: pull up. */
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
        }
        else if (mode == PIN_MODE_INPUT_PULLDOWN)
        {
            /* input setting: pull down. */
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        }
        else if (mode == PIN_MODE_OUTPUT_OD)
        {
            /* output setting: od. */
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
        }

        HAL_GPIO_Init(gpio_typ, &GPIO_InitStruct);
    }
        break;
    default:
        ASSERT(0);
        break;
    }
    return 0;
}

void driver_pin_init(void) {
    _pin_dev.parent.dev_ops.open = stm32_pin_open;
    _pin_dev.parent.dev_ops.close = stm32_pin_close;
    _pin_dev.parent.dev_ops.read = stm32_pin_read;
    _pin_dev.parent.dev_ops.write = stm32_pin_write;
    _pin_dev.parent.dev_ops.ioctl = stm32_pin_ioctl;
    device_register(&_pin_dev.parent, PIN_DEV_PATH);
}
