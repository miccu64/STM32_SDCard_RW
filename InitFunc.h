#include <stdint.h>

//UNCOMMENT FOR TRACE
/*
#define MISO GPIO_PIN_0
#define MOSI GPIO_PIN_1
#define SCLK GPIO_PIN_2//serial clock
#define CS GPIO_PIN_3//choose slave
#define MISOPORT GPIOF
#define MOSIPORT GPIOF
#define SCLKPORT GPIOF
#define CSPORT GPIOF
#define BLOCKSIZE 512
*/

#define MISO GPIO_PIN_9
#define MOSI GPIO_PIN_14
#define SCLK GPIO_PIN_8//serial clock
#define CS GPIO_PIN_2//choose slave
#define MISOPORT GPIOG
#define MOSIPORT GPIOG
#define SCLKPORT GPIOC
#define CSPORT GPIOB
#define BLOCKSIZE 512

void MyInit(void);
uint8_t InitSD(void);
