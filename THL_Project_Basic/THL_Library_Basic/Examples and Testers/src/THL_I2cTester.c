/*
 * THL_I2cTester.c
 *
 *  Created on: Feb 12, 2019
 *      Author: Hongtao Zhang
 */

#include "THL_I2cTester.h"

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

I2C i2cBusMem;
USART* system_console;
I2C* i2cBus;

GPIO ledMem;
GPIO* led;
GPIO buttonMem;
GPIO* button;



void setup(void) {
	system_console = newMainUSART(&huart2);
	i2cBus = newI2C(&i2cBusMem ,&hi2c1);
	led = newGPIO(&ledMem, LD2_GPIO_Port, LD2_Pin);
	button = newGPIO(&buttonMem, B1_GPIO_Port, B1_Pin);
}


#define Master_Board
//#define Slave_Board
#define Target_Address 6

void testTwoNucleoBoardComm(void) {
	setup();
#ifdef Master_Board
	//for(int i = 0; i<5; i++) blink(led, 300);
	gpioWrite(led, High);
	while(gpioRead(button));
	gpioWrite(led, Low);

	/*
	strcpy(i2cBus->TxBuffer, "Message A from the Master");
	i2cSend(i2cBus, MasterMode(Target_Address));
*/
/*
	gpioWrite(led, Low);
	strcpy(i2cBus->TxBuffer, "Message B from the Master");
	i2cSend_IT(i2cBus, MasterMode(Target_Address));
	while(i2cBus->TxStatus != Completed);
*/
	gpioWrite(led, Low);
	strcpy(i2cBus->TxBuffer, "Message C from the Master");
	i2cSend_DMA(i2cBus, MasterMode(Target_Address));
	while(i2cBus->TxStatus != Completed);

	for(int i = 0; i<5;i++) blink(led, 300);
	printf_u("\rMessage Sent!!!!!!!!!!!!!!!!!\r\n");
#endif
#ifdef Slave_Board

	printf_u("\r*******************************\r\n");
	gpioWrite(led, High);
	while(1) {
		i2cBus->RxTimeOut = 0xFFFF;
		i2cRead(i2cBus, SlaveMode, 25);
		//toggle(led);
		printf_u("\rReceived: \r\n");
		printf_u("\r%s\r\n", i2cBus->RxBuffer);
		gpioWrite(led, Low);
	}
#endif
}


void testI2c(void) {
	testTwoNucleoBoardComm();
}

void Exception_Handler(const char* str) {
	printf_u("\r%s\r\n",str);
}

