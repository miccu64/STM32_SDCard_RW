#include <stdint.h>
#include "stm32f4xx_hal_adc.h"

//initialize temp sensor (use ADC)
uint8_t InitADCTemp(ADC_HandleTypeDef* HandleADC);

//fills buffer with temerature value (and it converts float to bits and saves to uint32_t)
uint8_t FillBufferWithTemp (ADC_HandleTypeDef* HandleADC, uint8_t buf[], int size, float tempValues[]);
