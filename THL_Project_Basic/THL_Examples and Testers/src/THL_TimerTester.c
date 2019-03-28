/*
 * THL_TimerTester.c
 *
 *  Created on: Mar 26, 2019
 *      Author: zhang
 */

#include "THL_TimerTester.h"
#include "main.h"


extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim8;



TIM timer1Mem;
TIM* timer1;

TIM timer5Mem;
TIM* timer5;

TIM timer8Mem;
TIM* timer8;


USART* system_console;

GPIO ledMem;
GPIO* led;
GPIO buttonMem;
GPIO* button;



static void setup(void) {
	system_console = newMainUSART(&huart2);
	led = newGPIO(&ledMem, LD2_GPIO_Port, LD2_Pin);
	button = newGPIO(&buttonMem, B1_GPIO_Port, B1_Pin);
	printf_u("\rTimer Testing\r\n");
}




static void testPWM(void) {
	timer1 = newTIM(&timer1Mem, &htim1);
	initTIM_PWM(timer1, 10000, 10000); //max_cnt = 10,000; pwm_freq = 10k;
	timPwmGenBegin(timer1, TIM_CH1);
	timPwmGenBegin(timer1, TIM_CH2);
	timPwmGenBegin(timer1, TIM_CH3);
	timPwmGenBegin(timer1, TIM_CH4);

	gpioWrite(led, High);
	while(gpioRead(button));
	gpioWrite(led, Low);
	delay(500);
	gpioWrite(led, High);

	double dutyCircle = 10.00f, increm = 5.25f, gap = 10.00f, iter = 0.00f;
	double dc1, dc2, dc3, dc4;
	while(1) {

		if(!gpioRead(button)) {

			//Altering freq real-time
			if((uint32_t)iter % 2 == 1) timSetPwmFrequency(timer1, 10000, 10000);
			else timSetPwmFrequency(timer1, 10000, 100);

			dc1 = dutyCircle + 0.00f * gap + iter * increm;
			dc2 = dutyCircle + 1.00f * gap + iter * increm;
			dc3 = dutyCircle + 2.00f * gap + iter * increm;
			dc4 = dutyCircle + 3.00f * gap + iter * increm;
			if(dc1 > 100.00f) dc1 = dc1 - ((uint32_t)(dc1 / 100.00f) * 100.00f);
			if(dc2 > 100.00f) dc2 = dc2 - ((uint32_t)(dc2 / 100.00f) * 100.00f);
			if(dc3 > 100.00f) dc3 = dc3 - ((uint32_t)(dc3 / 100.00f) * 100.00f);
			if(dc4 > 100.00f) dc4 = dc4 - ((uint32_t)(dc4 / 100.00f) * 100.00f);


			timPwmWrite(timer1, TIM_CH1, dc1);
			timPwmWrite(timer1, TIM_CH2, dc2);
			timPwmWrite(timer1, TIM_CH3, dc3);
			timPwmWrite(timer1, TIM_CH4, dc4);

			iter += 1.00f;
			gpioWrite(led, Low);
			delay(1000);
			gpioWrite(led, High);
		}

/*
		timPwmWrite(timer1, TIM_CH1, 10.25);
		timPwmWrite(timer1, TIM_CH2, 30.59);
		timPwmWrite(timer1, TIM_CH3, 50.95);
		timPwmWrite(timer1, TIM_CH4, 75.33);*/
	}
}

void testTimer(void) {
	setup();
	testPWM();
}

