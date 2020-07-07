#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"

uint8_t InitADCTemp(ADC_HandleTypeDef* HandleADC) {
	__ADC1_CLK_ENABLE();
	
	ADC_InitTypeDef InitADC;
	
	InitADC.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	InitADC.ContinuousConvMode = ENABLE;//continous mode
	InitADC.DataAlign = ADC_DATAALIGN_RIGHT;//lsb is on 0
	InitADC.DiscontinuousConvMode = DISABLE;
	InitADC.DMAContinuousRequests = ENABLE;
	InitADC.EOCSelection = DISABLE;//ADC_EOC_SINGLE_CONV;
	InitADC.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
	InitADC.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	InitADC.NbrOfConversion = 2;
	InitADC.NbrOfDiscConversion = 0;
	InitADC.Resolution = ADC_RESOLUTION_12B;
	InitADC.ScanConvMode = DISABLE;
	
	HandleADC->Instance = ADC1;
	HandleADC->Init = InitADC;

	HAL_ADC_Init(HandleADC);
	
	ADC_ChannelConfTypeDef adcChannel;
	adcChannel.Channel = ADC_CHANNEL_18;
    adcChannel.Rank = 1;
    adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    adcChannel.Offset = 0;
 
    if (HAL_ADC_ConfigChannel(HandleADC, &adcChannel) != HAL_OK) {
        return 1;//config channel error
    } 
	
	return 0;
}
	
uint8_t FillBufferWithTemp (ADC_HandleTypeDef* HandleADC, uint8_t buf[], int size, float tempValues[]) {
	//fill buffer with temperature from ADC
	uint32_t value;
	float temp;
	
	//start continous conversion
	HAL_ADC_Start(HandleADC);
	for(int a=0; a<size/4; a++) {
		if (HAL_ADC_PollForConversion(HandleADC, 1000) == HAL_OK) {
			value = HAL_ADC_GetValue(HandleADC);
			//we need to convert value (values from datasheet and my knowledge)
			temp = (3.3*value)/4095.0;
			//temp = ((temp-0.76f)/0.0025f)+25.0f;
			temp = ((temp-0.76)/0.0025)+25.0;
			tempValues[a] = temp;
			
			value = *(int*)&temp;//needed to get binary representation of float
			for(int b=3; b>=0; b--) {
				//save 8 bits from float, which is 32 bits
				buf[4*a+3-b] = (value>>(b*8)) & 0xff;
			}
		} else return 1;//error of ADC
	} 
	//turn off ADC
	HAL_ADC_Stop(HandleADC);
	
	return 0;
}



