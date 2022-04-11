/*
 *      Author: ljp
 */

#include "stm32f4xx_hal.h"
#include "stdio.h"


#define BIT_MASK_FROM_TO(n1, n2) (((unsigned) -1 >> (31 - (n2))) & ~((1U << (n1)) - 1))
/*lv[n1..n2] = val*/
#define BIT_RANGE_ASSIGN(reg, val, n1, n2) ((reg) = ((reg) & ~(BIT_MASK_FROM_TO((n1),(n2)))) | ((((val) << (n1)) & (BIT_MASK_FROM_TO((n1),(n2))))))


void leds_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pins : PF7 PF8 PF9 PF10 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

////D6 - D9: PF7 - PF10
void led_set(unsigned int val) {
    BIT_RANGE_ASSIGN(GPIOF->ODR, val, 7, 10);
}


void keys_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  
  /*Configure GPIO pin : PI9 - K3*/
  __HAL_RCC_GPIOI_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PF11 - K4*/
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PC13 - K5*/
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  __HAL_RCC_GPIOC_CLK_ENABLE();
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 - K6*/
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

int k3_read(void) {
    return (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_9) == GPIO_PIN_SET);
}
int k4_read(void) {
    return (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_11) == GPIO_PIN_SET);
}
int k5_read(void) {
    return (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET);
}
int k6_read(void) {
    return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
}
