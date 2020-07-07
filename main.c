//Zapisywanie temperatury pobranej z procesora na karcie w formacie FatSD

#include "stm32f4xx_hal.h"
#include <string.h>
#include "InitFunc.h"
#include "SendReceive.h"
#include "TempSensor.h"
#include <stdint.h>
#include <stdbool.h>
//#include "trace.h"//for trace purposes

//size of buffers
#define BUFSIZE 1222
//if it stucks somewhere, it probably is fault of too big BUFSIZE (but should work even for 50k+)
#define SAVEBLOCKS 3//how much blocks do want to save
#define READBLOCKS 3//how much blocks do want to read

void SysTick_Handler(void) {
    HAL_IncTick();
}

//function decides if write only one block or more
uint8_t WriteData(uint8_t buf[], int size, int blockNumber, int howMuch) {
	uint8_t result;
	if(howMuch==1)
		result = WriteOneBlock(blockNumber,buf,size);
	else result = WriteMultipleBlocks(blockNumber, buf, size, howMuch);
	return result;
}

//function decides if read only one block or more
uint8_t ReadData(uint8_t buf[], int size, int blockNumber, int numOfBlocks) {
	uint8_t result;
	if(numOfBlocks==1)
		result = ReadOneBlock(blockNumber, buf, size);
	else result = ReadMultipleBlocks(blockNumber, numOfBlocks, buf, size);
	return result;
}

void ConvertData(uint8_t buf[], int size, float tempValues[]) {
	//converts data from float bits saved in uint32_t to float
	float temp;
	uint32_t help = 0;
	int count = 3;
	int index = 0;

	for(int b=0; b<size/BLOCKSIZE; b++) {
		for(int a=1; a<510; a++) {
			//avoid 0xff's
			if((a==4) && (b>0)) {
				while(buf[a]==0xff)
					a++;
				a++;//to avoid 0xfe(start byte)
			}
			//convert from bits to float
			help |= (buf[a]<<(count*8));
			count--;
			if(count == -1) {
				count = 3;
				temp = *(float*)(&help);
				tempValues[index] = temp;
				index++;
				help = 0;
			}
		}
	}
}

int main(void)
{
	//init what we need
	HAL_Init();
	volatile uint8_t res;
	ADC_HandleTypeDef HandleADC;
	
	res = InitADCTemp(&HandleADC);
	if(res != 0x00)
		return 1;//init adc error
	//TRACE_Init();//UNCOMMENT FOR TRACE
	MyInit();//2nd part of init sd card (on GPIO)
	
	res = InitSD();
	if(res != 0x00)
		return 2;//init sd error
	
	//specify size of data buffer and receive buffer
	static uint8_t buf[BUFSIZE] = {0};
	static float tempValues[BUFSIZE/4] = {0};
	
	
	
	//take values from temperature sensor
	res = FillBufferWithTemp(&HandleADC, buf, BUFSIZE, tempValues);
	if(res != 0x00)
		return 3;//filling buffer error
	
	//addresses are 0, 512, 1024, 1536, 2048...
	//but function takes 0, 1, 2... and mnozy by BLOCKSIZE
	res = WriteData(buf, BUFSIZE, 1, SAVEBLOCKS);
	if(res != 0x00)
		return 4;//write error
	
	//clear the buffers
	memset(buf, 0, sizeof(buf));
	memset(tempValues, 0, sizeof(tempValues));
	//read the data from sd
	res = ReadData(buf, BUFSIZE, 1, READBLOCKS);
	if(res != 0x00)
		return 5;//read error
	
	ConvertData(buf, BUFSIZE, tempValues);

	return 0;
} 
