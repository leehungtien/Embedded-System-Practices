/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

static void MX_GPIO_Init(void);
extern void initialise_monitor_handles(void);
void SystemClock_Config(void);

int counter = 0;
int T1, currentInterval, previousInterval = 0;

HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	int T2 = HAL_GetTick();
	currentInterval = T2 - T1;
	T1 = T2;
	counter++;
	printf("Counter is: %d\n", counter);
	// Check which pin is calling the function (Add conditional code)
	if(GPIO_Pin == BUTTON_EXTI13_Pin)
	{
		printf("\t Blue button is pressed. \n\n");

		if (HAL_GPIO_ReadPin(GPIOB, LED2_Pin) == GPIO_PIN_SET) {

			// Button pressed twice in less than 2 seconds
			if (currentInterval + previousInterval < 1000) {

				// We have to check for counter as the first time that the button is pressed and if it's within 1 second, it'll go through without this condition
				if (counter == 2) {
					HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
					printf("Time difference is: %d\n", currentInterval + previousInterval);
					counter = 0;
				}
			}
			else {
				counter = 0;
			}
		}

		// If LED is switched off, pressing the button once would have switched on the LED
		else {
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
			counter = 0;
		}

	}
}

int main(void)
{
	initialise_monitor_handles();
	HAL_Init();
	MX_GPIO_Init();
	T1 = HAL_GetTick();
}

static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();	// Enable AHB2 Bus for GPIOB
	__HAL_RCC_GPIOC_CLK_ENABLE();	// Enable AHB2 Bus for GPIOC

	HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET); // Reset the LED2_Pin as 0

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Configuration of LED2_Pin (GPIO-B Pin-14) as GPIO output
	GPIO_InitStruct.Pin = LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Configuration of BUTTON_EXTI13_Pin (GPIO-C Pin-13) as AF,
	GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Interrupt falling. Interrupt triggered whenever signal goes from 1 to 0 (falling). stm32I4xx_it.c file called automatically as long as there's an IT in the mode
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	// Enable NVIC EXTI line 13
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}
