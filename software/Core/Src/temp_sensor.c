/*
 * temp_sensor.c
 *
 *  Created on: Nov 10, 2020
 *      Author: Calvin Johnson
 */

#include "temp_sensor.h"
#include "tim.h"
#include <string.h>

//GPIO_TypeDef *ports[5] = { TEMP2_GPIO_Port/*TEMP1_GPIO_Port*/, TEMP2_GPIO_Port,
//		TEMP3_GPIO_Port, TEMP4_GPIO_Port, TEMP5_GPIO_Port };
//uint16_t pins[5] = { TEMP2_Pin/*TEMP1_Pin*/, TEMP2_Pin, TEMP3_Pin, TEMP4_Pin,
//		TEMP5_Pin };

// TODO: match this to physical number correctly
uint8_t num_temp_readings[4] = { 7, 9, 7, 7 };
uint8_t num_readings[4];
raw_temp_reading raw_temp_readings[4];

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	uint32_t channel1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
	uint32_t channel2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
	uint32_t channel4 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);

	//HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
	if (htim->Instance == TIM2) {
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
			// temp1
			if (num_readings[TEMP_LINE_1] < num_temp_readings[TEMP_LINE_1]) {
				raw_temp_readings[TEMP_LINE_1].times[num_readings[TEMP_LINE_1]] = channel1;
				num_readings[TEMP_LINE_1]++;
			}

		} else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
			// temp2
			//HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
			if (num_readings[TEMP_LINE_2] < num_temp_readings[TEMP_LINE_2]) {
				raw_temp_readings[TEMP_LINE_2].times[num_readings[TEMP_LINE_2]] = channel4;
				num_readings[TEMP_LINE_2]++;
			}
			//__HAL_TIM_SET_COUNTER(&htim2,0);

		} else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
			// temp3
			if (num_readings[TEMP_LINE_3] < num_temp_readings[TEMP_LINE_3]) {
				raw_temp_readings[TEMP_LINE_3].times[num_readings[TEMP_LINE_3]] = channel2;
				num_readings[TEMP_LINE_3]++;
			}
		}
	} else if (htim->Instance == TIM3) {
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
			// temp1
			if (num_readings[TEMP_LINE_4] < num_temp_readings[TEMP_LINE_4]) {
				raw_temp_readings[TEMP_LINE_4].times[num_readings[TEMP_LINE_4]] += channel1;
				//uint32_t timer_value = __HAL_TIM_GET_COUNTER(&htim3);
				num_readings[TEMP_LINE_4]++;
				if (num_readings[TEMP_LINE_4] < num_temp_readings[TEMP_LINE_4]) {
					//raw_temp_readings[TEMP_LINE_4].times[num_readings[TEMP_LINE_4]] =	timer_value;
					__HAL_TIM_SET_COUNTER(&htim3, 0);
				}

			}

		}
	}

}

void temp_sensor_init() {
	//HAL_TIM_RegisterCallback(&htim2, HAL_TIM_IC_CAPTURE_CB_ID, Timer2_IC_CaptureCallback);
	//HAL_TIM_RegisterCallback(&htim3, HAL_TIM_IC_CAPTURE_CB_ID, Timer3_IC_CaptureCallback);
}

temp_reading parse_temp_readings(raw_temp_reading raw_readings[4]) {
	temp_reading reading = { 0 };
	int temp_num = 0;
	for (int i = 0; i < 4; i++) {
		int num_temp_vals = (num_temp_readings[i] - 1) / 2;
		for (int j = 0; j < num_temp_vals; j++) {
			long th = 0;
			long tl = 0;
			if (i == 3) {
				th = raw_readings[i].times[j * 2 + 1];
				tl = raw_readings[i].times[j * 2 + 2];
			} else {
				th = raw_readings[i].times[j * 2 + 1] - raw_readings[i].times[j * 2];
				tl = raw_readings[i].times[j * 2 + 2] - raw_readings[i].times[j * 2 + 1];
			}
			if (tl == 0 || th == 0) {
				reading.temps[temp_num] = 0;
			} else {
				reading.temps[temp_num] = 421 - (751 * ((double) th / tl));
			}
			temp_num++;

		}
	}
	return reading;
}

void delay_us(uint16_t us) {
	__HAL_TIM_SET_COUNTER(&htim1, 0);  // set the counter value to 0
	while (__HAL_TIM_GET_COUNTER(&htim1) < (us)) {
	}  // wait for the counter to reach the us input in the parameter
}

void get_temp_reading() {
	// clear temp 4 to be all 0s since we'll be fiddling with it
	memset(raw_temp_readings[TEMP_LINE_4].times, 0, sizeof(long) * MAX_NUM_READINGS);

	HAL_TIM_Base_Start(&htim1);

	num_readings[0] = 1;
	num_readings[1] = 1;
	num_readings[2] = 1;
	num_readings[3] = 1;

	// set low
	HAL_GPIO_WritePin(TEMP_SOC_GPIO_Port, TEMP_SOC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	delay_us(20);

	// set high
	HAL_GPIO_WritePin(TEMP_SOC_GPIO_Port, TEMP_SOC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
	//HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
	delay_us(20);

	// set low - start reading
	HAL_GPIO_WritePin(TEMP_SOC_GPIO_Port, TEMP_SOC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	//HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);

	// start channel interrupts
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
	raw_temp_readings[TEMP_LINE_1].times[0] = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
	raw_temp_readings[TEMP_LINE_2].times[0] = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_4);
	raw_temp_readings[TEMP_LINE_3].times[0] = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_2);
	raw_temp_readings[TEMP_LINE_4].times[0] = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);

	// delay till got all readings
	while ((num_readings[TEMP_LINE_2] < num_temp_readings[TEMP_LINE_2])) {
	}

	HAL_Delay(10);

	// raise pin high to signify finished
	HAL_GPIO_WritePin(TEMP_SOC_GPIO_Port, TEMP_SOC_Pin, GPIO_PIN_SET);

	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_2);
	HAL_TIM_IC_Stop_IT(&htim3, TIM_CHANNEL_1);
}

