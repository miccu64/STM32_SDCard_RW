#include "stm32f4xx_hal.h"
#include "InitFunc.h"
#include "SendReceive.h"
#include <stdbool.h>

uint8_t InitSD(void) {
	HAL_Delay(10);
	//MOSI and CS need to be in high state
	HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);
	HAL_Delay(10);//we have to wait for wake up of sdcard
	
	//change to spi mode
	//we need minimum 74 cycles to send
	for(int a=0; a<10; a++)//10*8=80 cycles
		SendDummyByte();
	HAL_Delay(10);
	
	volatile uint8_t response = 0xff;
	volatile uint32_t longResponse = 0xffffffff;
	
	for(int b=0; b<10; b++) {
		response = SendCMD(0, 0, 0x95);//init sd card
		if(response == 0x01) {
			break;//is okay
		}
		HAL_Delay(100);//give time for init
	}
	if(response != 0x01) {
		return 10;//init error
	}
	
	//check if sdcard needs to set its blocksize (sdhc doesn't need)
	response = SendCMD(58, 0, 0xfd);
	longResponse = Get2ndPartResponse();
	volatile bool needsBlockSize = true;
	if(((longResponse >> 29) & 0x01) == 1)//check the bit[30]
		needsBlockSize = false;//doesn't need to specify block size
	
	//check version of sd card
	response = SendCMD(8, 0x000001AA, 0x87);
	longResponse = Get2ndPartResponse();
	if(response == 0xff) {
		return 2;//checking version of sdcard error
	}	
	
	//0x05 = ver1 of sdcard, 0x01 = ver2
	//0x01 = newer init by ACMD
	if (response == 0x01 && longResponse == 0x000001AA) {
		for(int a=0; a<10; a++) {
			response = SendACMD(41, 0x40000000, 0x65);
			if(response == 0x00)
				break;
			HAL_Delay(100);//it needs at least 100ms of delay
		}
	}
	
	//other cases = try to use ACMD with other values to init
	if (response != 0x00) {
		for(int a=0; a<10; a++) {
			//try to init with ACMD with values for older sdcards
			response = SendACMD(41, 0, 0xE5);//needs to set bit[30]
			if(response == 0x00)
				break;
			HAL_Delay(100);//it needs at least 100ms of delay
		}
	}
	
	//other cases = try to use CMD1 to init
	if (response != 0x00) {
		for(int a=0; a<10; a++) {
			//try to init with worse command for older sdcards
			response = SendCMD(1, 0, 0xF9);
			if(response == 0x00)
				break;
			HAL_Delay(100);//it needs at least 100ms of delay
		}
	}
	
	if(response != 0x00) {
		return 3;//init error
	}
	
	//set block size if needed
	if(needsBlockSize==true) {
		response = SendCMD(16, BLOCKSIZE, 0xff);
		if(response != 0x00)
			return 4;//setting block size error
	}
	
	return 0;
}

void MyInit(void) {
	GPIO_InitTypeDef InitGPIO;
	
	// GPIO Ports Clock Enable
	//__GPIOF_CLK_ENABLE();//UNCOMMENT FOR TRACE

	__GPIOB_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	__GPIOG_CLK_ENABLE();

	InitGPIO.Pin = CS;
	InitGPIO.Mode = GPIO_MODE_OUTPUT_PP;
	InitGPIO.Pull = GPIO_NOPULL;
	InitGPIO.Speed = GPIO_SPEED_LOW;
	InitGPIO.Alternate = 0;
	HAL_GPIO_Init(CSPORT, &InitGPIO);

	InitGPIO.Pin = SCLK;
	InitGPIO.Mode = GPIO_MODE_OUTPUT_PP;
	InitGPIO.Pull = GPIO_NOPULL;
	InitGPIO.Speed	= GPIO_SPEED_LOW;
	InitGPIO.Alternate = 0;
	HAL_GPIO_Init(SCLKPORT, &InitGPIO);
	
	InitGPIO.Pin = MOSI;
	InitGPIO.Mode = GPIO_MODE_OUTPUT_PP;
	InitGPIO.Pull = GPIO_NOPULL;
	InitGPIO.Speed = GPIO_SPEED_LOW;
	InitGPIO.Alternate = 0;
	HAL_GPIO_Init(MOSIPORT, &InitGPIO);
	
	InitGPIO.Pin = MISO;
	InitGPIO.Mode = GPIO_MODE_INPUT;
	InitGPIO.Pull = GPIO_PULLUP;
	InitGPIO.Speed = GPIO_SPEED_LOW;
	InitGPIO.Alternate = 0;
	HAL_GPIO_Init(MOSIPORT, &InitGPIO);

	//UNCOMMENT FOR TRACE
	//GPIO_InitStruct.Pin   = MOSI | MISO | CS | SCLK;
	//HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//needed
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);//needed
	HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);//needed
	
}
