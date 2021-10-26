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
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_hsensor.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_gyro.h"
#include "stdio.h"
#include "string.h"

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf)
static void UART1_Init(void);
UART_HandleTypeDef huart1;

static void MX_GPIO_Init(void); //initialize LED2_Pin
void SystemClock_Config(void);

void temp_monitoring(int *blink_flag_5Hz, int *temp_flag, int *temp_T1, int *temp_T2, float *temp_data);
void pain_detection(int *blink_flag_10Hz, int *pain_flag, int *pain_T1, int *pain_T2);
void fall_detection(int *blink_flag_5Hz, int *fall_flag, int *fall_T1, int *fall_T2);
void orient_monitoring(int *blink_flag_10Hz, int *orient_flag, int *magneto_T1, int *magneto_T2);
void respiratory_monitoring(int *blink_flag_10Hz, int *respiratory_flag, int *respi_T1, int *respi_T2);

int press_counter = 0; //the number of times that MODE_TOGGLE is pressed
int Time2; //the time of the second time when MODE_TOGGLE is pressed
int Time1; //the time of the first time when MODE_TOGGLE is pressed
int Time_interval;//Time interval within the double press of MODE_TOGGLE
int MODE = 0; //Normal mode has MODE = 0, Intensive care mode has MODE = 1
int change_mode_flag = 0; //change_mode_flag = 1 when changing mode
//Mode flags (default as set when entering the mode. Reset after displaying the enter message once)
int Normal_flag = 1;
int Intensive_flag = 1;

//Detect double press of MODE_TOGGLE in Intensive mode to switch back to Normal mode
HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == BUTTON_EXTI13_Pin) //check is it pin13 interrupt that is calling this function
	{
		press_counter++;
		if (MODE == 1) //if the mode is intensive care mode
		{
			if (press_counter == 1) // first time pressing the MODE_TOGGLE
			{
				Time1 = HAL_GetTick();
			}
			else if (press_counter == 2) // second time pressing the MODE_TOGGLE
			{
				Time2 = HAL_GetTick();
				Time_interval = Time2 - Time1;
				if (Time_interval <= 500) // if two press within 0.5s
				{
					change_mode_flag = 1;
					Normal_flag = 1;
					MODE = 0; //reset MODE to 0 (Normal mode)
					press_counter = 0; //reset counter
				}
				else //Time_interval > 1s, the second press is now the first press
				{
					press_counter = 1;
					Time1 = Time2;
				}
			}
		}
	}

}

int main(void)
{
	initialise_monitor_handles(); // for semi-hosting support (printf)

	/* Reset of all peripherals, Initializes the Flash interface, the Systick and the GPIO. */
	HAL_Init();
	UART1_Init();
	MX_GPIO_Init();

	/* Peripheral initializations using BSP functions */
	BSP_ACCELERO_Init();
	BSP_TSENSOR_Init();
	BSP_MAGNETO_Init();
	BSP_PSENSOR_Init();
	BSP_HSENSOR_Init();
	BSP_GYRO_Init();

	//keep track of time interval between values being sent to Tera Term
	int value_T1 = 0;
	int value_T2 = 0;
	//keep track of time interval between the blinks of LED
	int B5Hz_T1 = 0;
	int B5Hz_T2 = 0;
	int B5Hz_counter = 0;
	int B10Hz_T1 = 0;
	int B10Hz_T2 = 0;
	int B10Hz_counter = 0;

	//keep track of time interval between the warnings
	int temp_T1 = 0;
	int temp_T2 = 0;
	int pain_T1 = 0;
	int pain_T2 = 0;
	int fall_T1 = 0;
	int fall_T2 = 0;
	int magneto_T1 = 0;
	int magneto_T2 = 0;
	int respi_T1 = 0;
	int respi_T2 = 0;

	//flags
	int blink_flag_5Hz = 0;
	int blink_flag_10Hz = 0;
	int temp_flag = 0;
	int fall_flag = 0;
	int respiratory_flag = 0;
	int pain_flag = 0;
	int orient_flag = 0;

	//sensors' readings
	float temp_data;

	char *Normal_start = "Entering Normal Mode.\r\n";
	char *Intensive_start = "Entering Intensive Care Mode.\r\n";

	int XXX = 0;

	while (1){
		if (change_mode_flag == 1){ // when changing mode
			//reset all flags
			blink_flag_5Hz = 0;
			blink_flag_10Hz = 0;
			temp_flag = 0;
			fall_flag = 0;
			respiratory_flag = 0;
			pain_flag = 0;
			orient_flag = 0;
			Normal_flag = 1;
			Intensive_flag = 1;


			if (MODE == 1){//get the time when the mode changes from Normal to Intensive
				value_T1 = HAL_GetTick();
			}
			change_mode_flag = 0;
		}

		if(MODE == 0){ //Normal mode
			if (Normal_flag == 1){ //display enter message once after entering the mode
				HAL_UART_Transmit(&huart1, (uint8_t*)Normal_start, strlen(Normal_start),0xFFFF);
				Normal_flag = 0;
			}

			temp_monitoring(&blink_flag_5Hz, &temp_flag, &temp_T1, &temp_T2, &temp_data);
			fall_detection(&blink_flag_5Hz, &fall_flag, &fall_T1, &fall_T2);
			respiratory_monitoring(&blink_flag_10Hz, &respiratory_flag, &respi_T1, &respi_T2);
			orient_monitoring(&blink_flag_10Hz, &orient_flag, &magneto_T1, &magneto_T2);

		}
		else if (MODE == 1){ //Intensive care mode
			if (Intensive_flag == 1){ //display enter message once after entering the mode
				HAL_UART_Transmit(&huart1, (uint8_t*)Intensive_start, strlen(Intensive_start),0xFFFF);
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET); //LED2 is ON when entering intensive mode
				Intensive_flag = 0;
			}

			temp_monitoring(&blink_flag_5Hz, &temp_flag, &temp_T1, &temp_T2, &temp_data);
			fall_detection(&blink_flag_5Hz, &fall_flag, &fall_T1, &fall_T2);
			respiratory_monitoring(&blink_flag_10Hz, &respiratory_flag, &respi_T1, &respi_T2);
			pain_detection(&blink_flag_10Hz, &pain_flag, &pain_T1, &pain_T2);
			orient_monitoring(&blink_flag_10Hz, &orient_flag, &magneto_T1, &magneto_T2);

//			value_T2 = HAL_GetTick();
//			if ((value_T2-value_T1)>=10000){
//				char value1_print[64]={0}; // UART transmit buffer
//				char value2_print[64]={0}; //any way to reuse one buffer instead of declearing 3 buffers??
//				char value3_print[64]={0};
//				sprintf(value1_print, "%03f TEMP_%2.2f (degreeC) ACC_x.xx(g)_y.yy(g)_z.zz(g)  \r\n", XXX, temp_data);
//				HAL_UART_Transmit(&huart1, (uint8_t*)value1_print, strlen(value1_print),0xFFFF);
//				sprintf(value2_print, "%03d GYRO nnn.n() MAGNETO X.XX() Y.YY() Z.ZZ() \r\n", XXX);
//				HAL_UART_Transmit(&huart1, (uint8_t*)value2_print, strlen(value2_print),0xFFFF);
//				sprintf(value3_print, "%03d HUMIDITY h.hh() and PRESSURE p.pp() \r\n", XXX);
//				HAL_UART_Transmit(&huart1, (uint8_t*)value3_print, strlen(value3_print),0xFFFF);
//				value_T1 = value_T2;
//				XXX++;
//			}

		}

		if(blink_flag_10Hz == 1){
			if(B10Hz_counter == 0){
				B10Hz_T1 = HAL_GetTick();
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET); //turn on LED2
				B10Hz_counter++;
			}
			B10Hz_T2 = HAL_GetTick();
			if((B10Hz_T2-B10Hz_T1)>= 50){ //toggle LED2_pin every 50ms (10Hz)
				if(HAL_GPIO_ReadPin(GPIOB, LED2_Pin)== GPIO_PIN_SET){
					HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
				}
				else if(HAL_GPIO_ReadPin(GPIOB, LED2_Pin)== GPIO_PIN_RESET){
					HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
				}
				B10Hz_T1 = B10Hz_T2;

			}
		}
		else if(blink_flag_5Hz == 1){
			if(B5Hz_counter == 0){
				B5Hz_T1 = HAL_GetTick();
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET); //turn on LED2
				B5Hz_counter++;
			}
			B5Hz_T2 = HAL_GetTick();
			if((B5Hz_T2-B5Hz_T1)>= 100){ //toggle LED2_pin every 100ms (5Hz)
				if(HAL_GPIO_ReadPin(GPIOB, LED2_Pin)== GPIO_PIN_SET){
					HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
				}
				else if(HAL_GPIO_ReadPin(GPIOB, LED2_Pin)== GPIO_PIN_RESET){
					HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
				}
				B5Hz_T1 = B5Hz_T2;
			}
		}

	}
}

#define TEMP_THRESHOLD 30

void temp_monitoring(int *blink_flag_5Hz, int *temp_flag, int *temp_T1, int *temp_T2, float *temp_data)
{
	// Get readings from temperature sensor
	*temp_data = BSP_TSENSOR_ReadTemp();	// read temperature sensor
	printf("%f\n", *temp_data);

	// When temperature sensor reading passes threshold for the very first time
	if (*temp_data >= TEMP_THRESHOLD && *temp_flag == 0) {
		*temp_T1 = HAL_GetTick();
		*temp_flag = 1;
		*blink_flag_5Hz = 1;

		// Transmit warning message to Tera Term
		char *warningMessage = "Fever is detected\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
	}
	else if (*temp_data >= TEMP_THRESHOLD || *temp_flag == 1) {
		*temp_T2 = HAL_GetTick();
		if ((*temp_T2 - *temp_T1) >= 10000){
			// Transmit warning message to Tera Term every 10s
			char *warningMessage = "Fever is detected\r\n";
			HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
			*temp_T1 = *temp_T2;
		}
	}
}

#define PAIN_THRESHOLD1 100000
#define PAIN_THRESHOLD2 -100000

void pain_detection(int *blink_flag_10Hz, int *pain_flag, int *pain_T1, int *pain_T2)
{
	// Get readings from gyroscope sensor
	float pain_data;
	BSP_GYRO_GetXYZ(&pain_data);	// read gyroscope sensor

	// When temperature sensor reading passes threshold for the very first time
	if ((pain_data >= PAIN_THRESHOLD1 || pain_data <= PAIN_THRESHOLD2) && *pain_flag == 0) {
		*pain_T1 = HAL_GetTick();
		*pain_flag = 1;
		*blink_flag_10Hz = 1;

		// Transmit warning message to Tera Term
		char *warningMessage = "Patient in pain! \r\n";
		HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
	}
	else if (pain_data >= PAIN_THRESHOLD1 || pain_data <= PAIN_THRESHOLD2 || *pain_flag == 1) {
		*pain_T2 = HAL_GetTick();
		if ((*pain_T2 - *pain_T1) >= 10000){
			// Transmit warning message to Tera Term every 10s
			char *warningMessage = "Patient in pain! \r\n";
			HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
			*pain_T1 = *pain_T2;
		}
	}
}

#define ACC_THRESHOLD 11

void fall_detection(int *blink_flag_5Hz, int *fall_flag, int *fall_T1, int *fall_T2)
{
	// Get readings from accelerometer
	float accel_data[3];
	int16_t accel_data_i16[3] = { 0 };
	BSP_ACCELERO_AccGetXYZ(accel_data_i16);

	// Function above returns values in milliGravity. We divide values by 100 to get ms^-2 since milliGravity = 10^-3 * 10ms^-2 = 10^-2 ms^-2
	accel_data[0] = (float)accel_data_i16[0] / 100.0f; // X value
	accel_data[1] = (float)accel_data_i16[1] / 100.0f; // Y Value
	accel_data[2] = (float)accel_data_i16[2] / 100.0f; // Z Value

	// When accelerometer reading passes threshold for the very first time
	if (accel_data[2] >= ACC_THRESHOLD && *fall_flag == 0) {
		*fall_T1 = HAL_GetTick();
		*fall_flag = 1;
		*blink_flag_5Hz = 1;

		// Transmit warning message to Tera Term
		char *warningMessage = "Fall detected\n";
		HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
	}

	else if (accel_data[2] >= ACC_THRESHOLD || *fall_flag == 1) {
		*fall_T2 = HAL_GetTick();
		if ((*fall_T2 - *fall_T1) >= 10000) {

			// Transmit warning message to Tera Term every 10s
			char *warningMessage = "Fall detected\n";
			HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
			*fall_T1 = *fall_T2;
		}
	}
}

#define MAG_THRESHOLD 0

void orient_monitoring(int *blink_flag_10Hz, int *orient_flag, int *magneto_T1, int *magneto_T2)
{
	// Get readings from MAGNETOMETER
	float magneto_data[3];
	int16_t magneto_data_i16[3] = { 0 };

	// Values in mGauss = 10^-3 Gauss. Therefore we divide by 1000
	BSP_MAGNETO_GetXYZ(magneto_data_i16);
	magneto_data[0] = (float)magneto_data_i16[0] / 1000.0f;
	magneto_data[1] = (float)magneto_data_i16[1] / 1000.0f;
	magneto_data[2] = (float)magneto_data_i16[2] / 1000.0f;

	// When MAGNETOMETER reading passes threshold for the very first time
	if (magneto_data[2] >= MAG_THRESHOLD && *orient_flag == 0) {
		*magneto_T1 = HAL_GetTick();
		*orient_flag = 1;
		*blink_flag_10Hz = 1;

		// Transmit warning message to Tera Term
		char *warningMessage = "Check patient's abnormal orientation!\n";
		HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
	}

	else if (magneto_data[2] >= MAG_THRESHOLD || *orient_flag == 1) {
		*magneto_T2 = HAL_GetTick();
		if ((*magneto_T2 - *magneto_T1) >= 10000) {

			// Transmit warning message to Tera Term every 10s
			char *warningMessage = "Check patient's abnormal orientation!\n";
			HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
			*magneto_T1 = *magneto_T2;
		}
	}
}

#define HUMID_THRESHOLD 80
#define PRESSURE_THRESHOLD 1000

void respiratory_monitoring(int *blink_flag_10Hz, int *respiratory_flag, int *respi_T1, int *respi_T2)
{
	// Unit: hPa
	float pressure_data;
	pressure_data = BSP_PSENSOR_ReadPressure();

	// Unit: %rH
	float humidity_data;
	humidity_data = BSP_HSENSOR_ReadHumidity();

	// When PRESSURE_SENSOR reading passes threshold OR HUMIDITY_SENSOR is below threshold for the very first time
	if ((pressure_data >= PRESSURE_THRESHOLD && *respiratory_flag == 0) || (humidity_data < HUMID_THRESHOLD && *respiratory_flag == 0)) {
		*respi_T1 = HAL_GetTick();
		*respiratory_flag = 1;
		*blink_flag_10Hz = 1;

		// Transmit warning message to Tera Term
		char *warningMessage = "Check patient's breath!\n";
		HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);

		// Change to Intensive Care Mode
		if (MODE == 0) {
			MODE = 1;
			Intensive_flag = 1;
			change_mode_flag = 1;
		}
	}

	else if ((pressure_data >= PRESSURE_THRESHOLD || *respiratory_flag == 1) || (humidity_data < HUMID_THRESHOLD || *respiratory_flag == 1)) {
		*respi_T2 = HAL_GetTick();
		if ((*respi_T2 - *respi_T1) >= 10000) {

			// Transmit warning message to Tera Term every 10s
			char *warningMessage = "Check patient's breath!\n";
			HAL_UART_Transmit(&huart1, (uint8_t*)warningMessage, strlen(warningMessage),0xFFFF);
			*respi_T1 = *respi_T2;
		}
	}
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

static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();	// Enable AHB2 Bus for GPIOB
	__HAL_RCC_GPIOC_CLK_ENABLE();	// Enable AHB2 Bus for GPIOC

	HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET); // GPIO_PIN_SET set the LED2_Pin as 0

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Configuration of LED2_Pin (GPIO-B Pin-14) as GPIO output
	GPIO_InitStruct.Pin = LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Configuration of BUTTON_EXTI13_Pin (GPIO-C Pin-13) as AF,
	GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;//interrupt triggers whenever signals goes from high 1 to low 0. use RISING for 0 to 1
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	// Enable NVIC EXTI line 13 (handler). Else, the interrupt won't be triggered.
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}
