#include "stm32f4xx_hal.h"
#include "InitFunc.h"
#include "SendReceive.h"
#include "math.h"
#include <stdint.h>
#include <stdbool.h>

void TransmitByte(uint8_t help) {
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
	for(int b=7; b>=0; b--) {
		uint8_t bit = ((help >> b) & 0x01);//take the b bit
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
		if(bit == 1)
			HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);
		else HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_SET);
	}
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
}

bool WaitFor0 () {
	//function waits to find start of response (so it wait for zero)
	uint8_t bit = 1;
	int i=0;
	while(bit == 1 && i<100) {	
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_SET);
		bit = HAL_GPIO_ReadPin(MISOPORT, MISO);	
		i++;
	}
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
	if(bit == 0)
		return true;
	return false;
}

uint8_t GetByteFromMISO(bool was0) {
	//was0 means that we need to add 0 on the start and get only 7 bits
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
	uint8_t response = 0xff;
	int a=7;
	if(was0 == true) {
		a--;
		response &= ~(1<<7);//zero MSB bit
	}
	for(; a>=0; a--) {	
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_SET);
		uint8_t bit = HAL_GPIO_ReadPin(MISOPORT, MISO);
		response = (response & ~(1UL << a)) | (bit << a);//change specific bit only	
	}
	HAL_GPIO_WritePin(SCLKPORT, SCLK, GPIO_PIN_RESET);
	return response;
}

void SendDummyByte(void) {
	TransmitByte(0xFF);
}

uint8_t GetResponse(void) {
	//MOSI must be in high state when reading
	HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);
	
	HAL_Delay(1);
	
	//get status byte
	//format:0xxxxxxx, x'es stand as error code - returns 0 if is okay
	//before end of initialization is this set to 1 (idle state)
	uint8_t response = 0xff;
	bool start = WaitFor0();//wait for start of a response
	if(start == true)
		response = GetByteFromMISO(start);
	return response;
}

uint32_t Get2ndPartResponse(void) {
	//MOSI must be in high state when reading, CS low
	HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);
	
	HAL_Delay(1);

	uint32_t response = 0xffffffff;
	int i=0;
	
	while(1) {	
		for (int a=3; a>=0; a--) {
			int shift = a*8;
			uint8_t res = GetByteFromMISO(false);
			//place bits in right place
			response = ((response & (~(0xff << shift))) | (res << shift));
		}
		i++;
		if(i == 4 || response != 0xffffffff) {
			break;
		}
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);
	}
	return response;
}

uint8_t SendCMD(uint8_t cmd, uint32_t arg, uint8_t crc) {
	HAL_GPIO_WritePin(MOSIPORT, MOSI, GPIO_PIN_SET);//needed
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);//reset CS pin to start transmit
	HAL_Delay(1);
	SendDummyByte();
	HAL_Delay(1);
	
	uint8_t buf[6];
	buf[0] = cmd | 0x40;//1st byte is a command code byte, next 4 are argument for that command
	buf[1] = (arg >> 24) & 0xff;
	buf[2] = (arg >> 16) & 0xff;
	buf[3] = (arg >> 8) & 0xff;
	buf[4] = arg & 0xff;
	buf[5] = crc;

	//transmit in little endian, STM32 uses little-endian	
	for(int a=0; a < 6; a++) {
		uint8_t help = buf[a];
		TransmitByte(help);
	}
	
	//get response after command
	uint8_t response = GetResponse();
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//set CS pin to end transmit
	HAL_Delay(1);
	
	return response;
}

uint8_t SendACMD(uint8_t acmd, uint32_t arg, uint8_t crc) {
	uint8_t response;
	response = SendCMD(55, 0, 0x65);
	response = SendCMD(acmd, arg, crc);
	return response;
}

uint8_t WriteOneBlock(uint8_t address, uint8_t buf[], int size) {
	//start writing one block of data
	uint8_t result = SendCMD(24,address*BLOCKSIZE,0xff);
	if(result != 0x00)
		return 1;
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);//set CS pin to start transmit
	HAL_Delay(1);
	//next we have to send data token 0xfe if one block we want to save
	TransmitByte(0xfe);
	
	//send data block
	for(int a=0; a<size; a++)
		TransmitByte(buf[a]);
	
	//needed 2 dummy bytes to end transmission (2 bytes of CRC)
	SendDummyByte();
	SendDummyByte();

	result = GetResponse();
	//three 1st bits are 1 in response, but GetResponse avoid 1 on the start, so we need to move it and add 111 on start
	result = result >> 3;
	result |= 0xe0;
	if(result == 0xe5)
		result = 0x00;
	else return result;
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//set CS pin to end transmit
	HAL_Delay(1);
	return result;
}

uint8_t ReadOneBlock(uint8_t address, uint8_t buf[], int size) {
	//send commant for read 1 block
	uint8_t result = SendCMD(17, address*BLOCKSIZE, 0xff);
	if(result != 0x00)
		return 1;
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);//reset CS pin to start transmit
	
	//get bytes
	for(int a=0; a<size; a++) {
		//uint32_t result2 = Get2ndPartResponse();
		uint8_t bb = GetByteFromMISO(false);
		buf[a] = bb;
	}
	
	SendDummyByte();
	SendDummyByte();
	SendDummyByte();
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//set CS pin to end transmit
	
	return result;
}

uint8_t WriteMultipleBlocks(uint8_t address, uint8_t buf[], int size, int howMuch) {
	//start saving with CMD25
	uint8_t result = SendCMD(25,address*BLOCKSIZE,0xff);
	
	if(result != 0x00)
		return 1;//error
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);//set CS pin to start transmit
	HAL_Delay(1);
	
	for (int y = 0; y<howMuch; y++) {
		//next we have to send data token for multiple blocks before every block
		TransmitByte(0xfc);
		
		for(int x=0; x<BLOCKSIZE-3; x++) {
			//send data block
			if(x+y*BLOCKSIZE >= size)
				TransmitByte(0x00);//if buf has ended, send clean bytes
			else TransmitByte(buf[x+y*BLOCKSIZE]);
		}
		
		//needed 2 dummy bytes to end transmission of block (2 bytes of CRC)
		SendDummyByte();
		SendDummyByte();
		
		//after every block we need to get response
		result = GetResponse();
		//three 1st bits are 1 in response, but GetResponse avoid 1 on the start, so we need to move it and add 111 on start
		result = result >> 3;
		result |= 0xe0;
		if(result != 0xe5)
			return 2;//write error
		
		//after every block we need to get response saying that it isn't busy state
		for(int a=0; a<100; a++) {
			result = GetResponse();
			if(result != 0xff && result != 0x00)
				break;
		}
		if(result == 0xff && result == 0x00)
			return 3;//wait error
	}
	
	TransmitByte(0xfd);//stop token to end saving blocks
	SendDummyByte();
	SendDummyByte();
	SendDummyByte();
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//set CS pin to end transmit
	HAL_Delay(1);
	return 0;
}

uint8_t ReadMultipleBlocks(uint8_t from, uint8_t howMuch, uint8_t buf[], int size) {
	//send command for reading blocks
	uint8_t result = SendCMD(18, from*BLOCKSIZE, 0xff);
	if(result != 0x00)
		return 1;
	
	HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_RESET);//reset CS pin to start transmit
	//next we have to send data token 0xfe if one block we want to read
	TransmitByte(0xfe);
	
	//get bytes
	for (int b=0; b<howMuch; b++) {
		for(int a=0; a<BLOCKSIZE; a++) {
			uint8_t bb = GetByteFromMISO(false);
			buf[a+b*BLOCKSIZE] = bb;
		}
	}
	
	//send command ending reading
	result = SendCMD(12, 0, 0xff);
	//we get stuff bytes, so we have to wait longer for response
	//we are waiting to stop getting 0es and get only 0xff
	for(int a=0; a<50; a++) {
		result = GetResponse();
		//we need to wait for 0xff on MISO
		if(result == 0xff) {
			//everything is okay, we can end without error
			SendDummyByte();
			SendDummyByte();
			SendDummyByte();
			HAL_GPIO_WritePin(CSPORT, CS, GPIO_PIN_SET);//set CS pin to end transmit
			return 0;
		}
	}
	return 2;//error
}
