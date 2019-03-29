#include "THL_Timer.h"


#ifdef HAL_TIM_MODULE_ENABLED

#define Max_Num_TIMs 20
uint16_t numActiveTIMs = 0;
TIM* ActiveTIMs[Max_Num_TIMs];

//Each section below has its own instantiation method


/*Abbreviation Notes:
 * ->ARR = Autoreload Register -- Period
 * ->CCR = Compare & Capture Register
 * ->CNT = Counter Value
 */


/*=======================Universal Function================================*/
/**This section is aimed for timer that multitasks, if a timer
  *is solely focus on e.g. pwm generation, go to PWM Generation
  *in another section below
  */
/**
 * Please use STM32CubeMx to config period and prescaler
 * Period & prescaler setting are applied on all four channels
 */
TIM *newTIM(TIM* instance, TIM_HandleTypeDef *htim, uint32_t APBx_DivFactor) {
	instance->htim = htim;
	instance->APBx_Div_Factor = APBx_DivFactor;
	for(int i = 0; i < numActiveTIMs; i++)
		if(ActiveTIMs[i]->htim == htim) {
			ActiveTIMs[i] = instance;
			return instance;
		}
	ActiveTIMs[numActiveTIMs++] = instance;
	return instance;
}

void timSetARR(TIM* instance, uint32_t ARR_val) {
	instance->ARR = ARR_val;
	__HAL_TIM_SET_AUTORELOAD(instance->htim, instance->ARR);
}

uint32_t timGetARR(TIM* instance) {
	instance->ARR = __HAL_TIM_GET_AUTORELOAD(instance->htim);
	return instance->ARR;
}

void timSetCCR(TIM* instance, uint32_t channel, uint32_t CCR_val) {
	instance->CCR = CCR_val;
	__HAL_TIM_SET_COMPARE(instance->htim, channel, CCR_val);
}

uint32_t timGetCCR(TIM* instance, uint32_t channel) {
	instance->CCR = __HAL_TIM_GET_COMPARE(instance->htim, channel);
	return instance->CCR;
}

void timSetCNT(TIM* instance, uint32_t CNT_val) {
	instance->CNT = CNT_val;
	__HAL_TIM_SET_COUNTER(instance->htim, CNT_val);
}

uint32_t timGetCNT(TIM* instance) {
	instance->CNT = __HAL_TIM_GET_COUNTER(instance->htim);
	return instance->CNT;
}

void timSetPrescaler(TIM* instance, uint32_t prescaler_val) {
	instance->TimerPrescaler = prescaler_val;
	__HAL_TIM_SET_PRESCALER(instance->htim, instance->TimerPrescaler);
}

uint32_t timGetPrescaler(TIM* instance) {
	return instance->TimerPrescaler;
}
/*===========================================================================*/


/*=======================Basic Counting & Interrupt==========================*/
uint32_t initTIM_BasicCounting(TIM* instance, uint32_t AutoReload_count, uint32_t timer_frequency) {
	timSetARR(instance, AutoReload_count);
	uint32_t ActualFreq = timSetFrequency(instance, timer_frequency);
	return ActualFreq;
}


uint32_t timSetFrequency(TIM* instance, uint32_t timer_frequency) {
	uint32_t TimerMaxFrequency = HAL_RCC_GetHCLKFreq() / instance->APBx_Div_Factor;

	if(timer_frequency > TimerMaxFrequency) {
		throwException("THL_Timer.c: timSetFrequency() | timer_frequency must be less or equal than TimerMaxFrequency");
		return Failed;
	}
	timSetPrescaler(instance, TimerMaxFrequency / timer_frequency - 1);

	//Prescaled frequency is subject to rounding error
	uint32_t ActualFrequency = TimerMaxFrequency / (timGetPrescaler(instance) + 1);
	return ActualFrequency;
}

void timCountBegin(TIM* instance) {
	HAL_TIM_Base_Start(instance->htim);
}
void timCountEnd(TIM* instance) {
	HAL_TIM_Base_Stop(instance->htim);
}


void timCountBegin_IT(TIM* instance) {
	HAL_TIM_Base_Start_IT(instance->htim);
}
void timCountEnd_IT(TIM* instance) {
	HAL_TIM_Base_Stop_IT(instance->htim);
}
uint32_t timGetCount(TIM* instance) {
	return timGetCNT(instance);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	for(int i = 0; i < numActiveTIMs; i++) {
		if(ActiveTIMs[i]->htim == htim) {
			timPE_IT_CallBack(ActiveTIMs[i]);
			timSysT_IT_CallBack(ActiveTIMs[i]);
		}
	}
}

__weak void timPE_IT_CallBack(TIM* instance) {
	UNUSED(instance);
}

__weak void timSysT_IT_CallBack(TIM* instance) {
	UNUSED(instance);
}
/*===========================================================================*/


/*=============================PWM Input/Output==============================*/


/** PWM Frequency Explaination
 *  - Timer frequency and PWM frequency are two distinct concepts!
 *    [SystemCoreClock] => [TimerClock/Freq] => [pwm_frequency]  (signals are produced from left to right).
 *    Timer clock is system clock divided by a division factor.
 *    Timer clock determines the frequency of each timer tick.
 *
 *    PWM signals are generated by setting a counter value <= ([Max_Count], a.k.a [timer period]), this
 *    counter value is what gets written to TIM->CCRx register. The timer output compare (a hardware function)
 *    pulls the signal line HIGH until the counter counts up to this TIM->CCRx value, and then pull it LOW till
 *    Max_Count. Technically, the [PwmDutyCircle] = [CCR] / [Max_Count] (in percentage).
 *
 * 	- The [TimerPrescaler] corresponds to a register that can be configured to set the desired timer frequency
 * 	  by dividing the "APB bus clock" scaled from [SystemCoreClock], a.k.a [HCLK_Frequency], with the TimerPrescaler value.
 * 	  Details of the clock relations can be looked up in STM32CubeMx software's clock configuration section,
 * 	  where a clock tree is presented.
 *
 * 	- [TimerMaxFrequency] = [HCLK] / [APBx_Bus_DivisionFactor] = [APBx_Bus_Frequency].
 *
 * 	- [TimerPrescaler] = [TimerMaxFrequency] / [TimerFrequency:(the desired frequency by user)].
 *
 * 	- [TimerFrequency:(desired)] = [Max_Count:(refer to 2nd paragraph)] * [pwm_frequency:(desired)].
 * 	  Basically the timer frequency is determined both by max count and pwm frequency,
 * 	  where the max count determines the resolution of this pwm signal.
 *
 * 	- [Max_Count] * [pwm_frequency] must be <= [TimerMaxFrequency], increasing PWM_Freq makes the driver
 * 	  respond quicker at the cost of resolution, while increasing the maximum counter value gives a higher
 * 	  resolution as the expense of responsiveness.
 *
 * 	- Turning the frequency down saves power consumption, but it usually dosen't matter.
 *
 * 	- For regular DC motor, 1kHZ-10kHZ is recommended in general.
 *
 * 	- For drone ESCs, frequency is much lower, refer to the ESC datasheet.
 *
 */

uint32_t initTIM_PWM(TIM* instance, uint32_t max_count, uint32_t pwm_frequency) {
	uint32_t rtn = timSetPwmFrequency(instance, max_count, pwm_frequency);
	return rtn;
}

/*Set PWM frequency at runtime*/
uint32_t timSetPwmFrequency(TIM* instance, uint32_t max_count, uint32_t pwm_frequency) {
	double TimerMaxFrequency = HAL_RCC_GetHCLKFreq() / instance->APBx_Div_Factor;
	double TimerFrequency = max_count * pwm_frequency;
	if(TimerFrequency > TimerMaxFrequency) {
		throwException("THL_Timer.c: setPwmFrequency() | max_count * pwm_frequency must be less or equal than TimerMaxFrequency");
		return Failed;
	}

	timSetPrescaler(instance, TimerMaxFrequency / TimerFrequency - 1);
	timSetARR(instance, ((uint32_t)TimerMaxFrequency / (instance->TimerPrescaler + 1) ) / pwm_frequency);
												//Minimize rounding error
	return Succeeded;
}

/**
 * @param  Channel TIM Channels to be enabled.
 *         This parameter can be one of the following values:
 *           @arg TIM_Channel_1
 *           @arg TIM_Channel_2
 *           @arg TIM_Channel_3
 *           @arg TIM_Channel_4
 */
void timPwmGenBegin(TIM* instance, uint32_t channel) {
	HAL_TIM_PWM_Start(instance->htim, channel);
	timSetPwmDutyCircle(instance, channel, 0);
}

void timPwmGenEnd(TIM* instance, uint32_t channel) {
	HAL_TIM_PWM_Stop(instance->htim, channel);
}

void timSetPwmDutyCircle(TIM* instance, uint32_t channel, uint32_t dutyCircleCnt) {
	timSetCCR(instance, channel, dutyCircleCnt);
}

//Pretty straightforward
void timPwmWrite(TIM* instance, uint32_t channel, double dutyCirclePercent) {
	dutyCirclePercent /= 100.00f;
	timSetPwmDutyCircle(instance, channel, (uint32_t)(dutyCirclePercent * (double)instance->ARR));
}
/*=========================================================================*/

/*=============================Input Capture===============================*/

/*=========================================================================*/




#ifdef fixthemlater


void setPwmDutyCircle(TIM_HandleTypeDef* htimx, uint32_t channel, uint32_t dutyCircleCnt) {
	//equivalent to htimx->instance->CCRx = dutyCircle
	__HAL_TIM_SET_COMPARE(htimx, channel, dutyCircle);
}

void pwm(TIM_HandleTypeDef* htimx, uint32_t channel, double dutyCirclePercent) {
	uint32_t cnt = (int)(dutyCirclePercent * (double)htimx->CounterPeriod);
	setPwmDutyCircle(htimx, channel, cnt);
}






#ifdef xxx
/***************************Input Capture*********************************/
volatile int PulseWidth[NumTimers + 1][NumChannel + 1] = {0};
volatile int PulseWidthtmp[NumTimers + 1][NumChannel + 1] = {0};

/*******Input Capture Blocking Mode**********/
volatile int *startInputCapture(TIM_HandleTypeDef* htimx, uint32_t channel) {
	if(HAL_TIM_IC_Start(htimx, channel) != HAL_OK) Error_Handler2();
	return PulseWidth[TIMx(htimx)];
}
int getTimCapturedVal(TIM_HandleTypeDef* htimx, uint32_t channel) {
	return (int)HAL_TIM_ReadCapturedValue(htimx, channel);
}
void MeasurePulseWidth(TIM_HandleTypeDef* htimx, uint32_t channel, uint32_t MaxDelay_us) {
	uint32_t t0 = micros(); 
	int tmp_IC_CapVal = getTimCapturedVal(htimx, channel);
	while(getTimCapturedVal(htimx, channel) == tmp_IC_CapVal) if(micros() - t0 > MaxDelay_us) return;
	tmp_IC_CapVal = getTimCapturedVal(htimx, channel);
	while(getTimCapturedVal(htimx, channel) == tmp_IC_CapVal) if(micros() - t0 > MaxDelay_us) return;
  PulseWidth[TIMx(htimx)][chDec(channel)] = getTimCapturedVal(htimx, channel) - tmp_IC_CapVal;
}
/********************************************/



/*******Input Capture Interrupt Mode*********/
volatile uint8_t TIM_IC_IT_index[NumTimers + 1][NumChannel + 1] = {0};
volatile uint8_t TIM_IC_IT_Channel[NumTimers + 1][NumChannel + 1] = {INACTIVE};
volatile int *startInputCapture_IT(TIM_HandleTypeDef* htimx, uint32_t channel) {
	if(HAL_TIM_IC_Start_IT(htimx, channel) != HAL_OK) Error_Handler2();
	TIM_IC_IT_index[TIMx(htimx)][chDec(channel)] = 0;
	TIM_IC_IT_Channel[TIMx(htimx)][chDec(channel)] = ACTIVE;
	return PulseWidth[TIMx(htimx)];
}
void stopInputCapture_IT(TIM_HandleTypeDef* htimx, uint32_t channel) {
	HAL_TIM_IC_Stop_IT(htimx, channel);
	TIM_IC_IT_Channel[TIMx(htimx)][chDec(channel)] = INACTIVE;
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htimx)
{
	for(int channel = 1; channel <= NumChannel; channel++) {
		if(TIM_IC_IT_Channel[TIMx(htimx)][channel] == INACTIVE) continue;
		if(TIM_IC_IT_index[TIMx(htimx)][channel] == 0) {
			PulseWidthtmp[TIMx(htimx)][channel] = (int)HAL_TIM_ReadCapturedValue(htimx, chEnc(channel));
			TIM_IC_IT_index[TIMx(htimx)][channel] = 1;
		}
		else if(TIM_IC_IT_index[TIMx(htimx)][channel] == 1) {
			PulseWidth[TIMx(htimx)][channel] = 
				(int)HAL_TIM_ReadCapturedValue(htimx, chEnc(channel)) - PulseWidthtmp[TIMx(htimx)][channel];
			TIM_IC_IT_index[TIMx(htimx)][channel] = 0;
		}			
	}
}
/**********************************************/
int getInputCapturePulseWidth(TIM_HandleTypeDef* htimx, uint32_t channel) {
	return PulseWidth[TIMx(htimx)][chDec(channel)];
}
int TIM_ICval(TIM_HandleTypeDef* htimx, uint32_t channel) {
	return PulseWidth[TIMx(htimx)][chDec(channel)];
}
uint8_t isChannelActive(TIM_HandleTypeDef* htimx, uint32_t channel) {
	return TIM_IC_IT_Channel[TIMx(htimx)][channel];
}
/*************************************************************************/

#endif
#endif

#endif


