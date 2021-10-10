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

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf)

void MX_GPIO_Init(void);

int main(void)
{
	initialise_monitor_handles(); // for semi-hosting support (printf)

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	MX_GPIO_Init();

	/* Peripheral initializations using BSP functions */
	BSP_ACCELERO_Init();
	BSP_TSENSOR_Init();
	BSP_MAGNETO_Init();
	BSP_PSENSOR_Init();

	int counter = 0 ;

	while (1) // Polling Mode
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		counter += 1;

		// When time is 1 second
		if (counter == 4) {
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

			printf("Accel X : %f; Accel Y : %f; Accel Z : %f; Magneto X: %f; Magneto Y: %f; Magneto Z: %f; Pressure: %f\n", accel_data[0], accel_data[1], accel_data[2], magneto_data[0], magneto_data[1], magneto_data[2], pressure_data);

		}

		else if (counter == 6) {
			float temp_data;
			temp_data = BSP_TSENSOR_ReadTemp();			// read temperature sensor
			printf("Temperature : %f\n", temp_data);

			// Reset counter
			counter = 0;
		}

		HAL_Delay(250);	// read once a ~second. Input is in milliseconds


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
}
