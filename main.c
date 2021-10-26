/******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * (c) EE2028 Teaching Team
  ******************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_accelero.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_magneto.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_psensor.h"
#include "stdio.h"
#include <string.h>

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf)

void MX_GPIO_Init(void);
static void UART1_Init(void);
UART_HandleTypeDef huart1;

int main(void)
{
	initialise_monitor_handles(); // for semi-hosting support (printf)

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	MX_GPIO_Init();
	UART1_Init();

	/* Peripheral initializations using BSP functions */
	BSP_ACCELERO_Init();
	BSP_TSENSOR_Init();
	BSP_MAGNETO_Init();
	BSP_PSENSOR_Init();

	int counter = 0;

	// set pbPressed to be 0 in the beginning to signify that the pushbutton wasn't pressed beforehand
	int pbPressed = 0;
	GPIO_PinState A;

	int currentDuration;
	int startTime = HAL_GetTick();
	while (1) // Polling Mode
	{
		currentDuration = HAL_GetTick() - startTime;
		if (A == 0) {

			// indicate that pbPressed
			pbPressed = !pbPressed;
		}

		// For every 250ms
		if (currentDuration % 250 == 0) {
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
			A = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
		}

		// For every 1 second
		if (currentDuration % 1000 == 0) {

			// If button is not pressed (i.e. print to terminal), pbPressed should equal to 0
			if (pbPressed == 0) {
				float accel_data[3];
				int16_t accel_data_i16[3] = { 0 };			// array to store the x, y and z readings.
				BSP_ACCELERO_AccGetXYZ(accel_data_i16);		// read accelerometer
				// the function above returns 16 bit integers which are 100 * acceleration_in_m/s2. Converting to float to print the actual acceleration. (in mGravity)
				accel_data[0] = (float)accel_data_i16[0] / 100.0f; // divide by 100 cause mGravity = 10^-3 * 10 = 10^-2
				accel_data[1] = (float)accel_data_i16[1] / 100.0f;
				accel_data[2] = (float)accel_data_i16[2] / 100.0f;

				float magneto_data[3];
				int16_t magneto_data_i16[3] = { 0 };
				BSP_MAGNETO_GetXYZ(magneto_data_i16); // values in mGauss = 10^-3 Gauss so divide by 1000
				magneto_data[0] = (float)magneto_data_i16[0] / 1000.0f;
				magneto_data[1] = (float)magneto_data_i16[1] / 1000.0f;
				magneto_data[2] = (float)magneto_data_i16[2] / 1000.0f;

				float pressure_data;
				pressure_data = BSP_PSENSOR_ReadPressure();

				// Message to be printed out
				char message[128];

				// Use sprintf to join all the variables into a string
				sprintf(message, "Accel X : %f; Accel Y : %f; Accel Z : %f; Magneto X: %f; Magneto Y: %f; Magneto Z: %f; Pressure: %f\n", accel_data[0], accel_data[1], accel_data[2], magneto_data[0], magneto_data[1], magneto_data[2], pressure_data);
				HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
			}
		}

		// For every 1.5 second
		if (currentDuration % 1500 == 0) {

			if (pbPressed == 0) {
				float temp_data;
				temp_data = BSP_TSENSOR_ReadTemp();			// read temperature sensor
				char message_temperature[32];
				sprintf(message_temperature, "Temperature : %f\n", temp_data);
				HAL_UART_Transmit(&huart1, (uint8_t*)message_temperature, strlen(message_temperature),0xFFFF);
			}
		}
	}

}

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Configure GPIO pin button
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void UART1_Init(void)
{
    /* Pin configuration for UART. BSP_COM_Init() can do this automatically */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configuring UART1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
      while(1);
    }

}
