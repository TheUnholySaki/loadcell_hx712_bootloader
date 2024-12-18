#include "test.h"
#include "main.h"

void PA17_test(void){
	while(1){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
		microDelay(500000);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
		microDelay(500000);
	}
}

