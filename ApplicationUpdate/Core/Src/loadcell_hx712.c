#include "loadcell_hx712.h"

/**
 * @brief Get data from ADC HX712
 * @param speed set sampling rate, 1 for 10HZ, 3 for 40Hz
 */
uint32_t getHX712(uint8_t speed)
{

  uint32_t data = 0;
  uint32_t tick = 0;
  while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_SET)
  {

    if (tick < 100000) tick++;
    else return 0;
  }
  for(int8_t len=0; len<24 ; len++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    microDelay(1);
    data = data << 1;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    microDelay(1);
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_SET)
      data ++;
  }

  for (int i = 0; i<speed; i++){
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	  microDelay(1);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	  microDelay(1);
  }

  data = data ^ 0x800000;

  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  
  return data;
}

