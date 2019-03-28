#ifndef __THL_TIMER_H
#define __THL_TIMER_H
#include "THL_Utility.h"
#include "THL_Portability.h"


#define TIM_CH1 TIM_CHANNEL_1
#define TIM_CH2 TIM_CHANNEL_2
#define TIM_CH3 TIM_CHANNEL_3
#define TIM_CH4 TIM_CHANNEL_4

#ifdef HAL_TIM_MODULE_ENABLED


typedef struct{
	TIM_HandleTypeDef *htim;
	uint32_t TimerPrescaler;
	uint32_t ARR; //ARR: (Counter) Auto Reload Register value, a.k.a timer period
				  //ARR = max_count = *htim->init->Period
	uint32_t CCR; //Capture/Compare (context dependent)
	uint32_t CNT;
}TIM;



/*=======================Universal Functions=================================*/
TIM *newTIM(TIM* instance, TIM_HandleTypeDef *htim);
void timSetARR(TIM* instance, uint32_t ARR_val);
void timSetCCR(TIM* instance, uint32_t channel, uint32_t CCR_val);
uint32_t timGetCCR(TIM* instance, uint32_t channel);
void timSetCNT(TIM* instance, uint32_t CNT_val);
uint32_t timGetCNT(TIM* instance);
void timSetPrescaler(TIM* instance, uint32_t prescaler_val);
/*===========================================================================*/



/*==============================PWM Generation===============================*/
uint32_t initTIM_PWM(TIM* instance, uint32_t max_count, uint32_t pwm_frequency);
uint32_t timSetPwmFrequency(TIM* instance, uint32_t max_count, uint32_t pwm_frequency);
void timPwmGenBegin(TIM* instance, uint32_t channel);
void timSetPwmDutyCircle(TIM* instance, uint32_t channel, uint32_t dutyCircleCnt);
void timPwmWrite(TIM* instance, uint32_t channel, double dutyCirclePercent);
/*===========================================================================*/

#endif
#endif