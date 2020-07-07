#include <stdint.h>

/*
Komendy:
CMD0	zeruje karte, pozwala wlaczyc tryb pracy z magistrala SPI
CMD1	inicjalizacja kart SDMMC
CMD8	sprawdza zakres napiec zasilania karty (wysyla sie argument 0x01aa i oczekuje takiej odpowiedzi)
CMD12	wymuszenie zakonczenia transmisji wielu bloków danych
CMD16	konfiguracja dlugosci bloku danych dla zapisu/odczytu
CMD17	odczyt bloku pamieci o dlugosci okreslonej przez CMD16
CMD24	zapis bloku pamieci o dlugosci okreslonej przez CMD16
CMD32	w argumencie jest przesylany adres pierwszego bloku przeznaczonego do skasowania
CMD33	w argumencie jest przesylany adres ostatniego bloku przeznaczonego do skasowania
CMD38	kasuje bloki wyznaczone za pomoca CMD32 i CMD33
CMD58
*/

//sends one byte through MOSI
void TransmitByte(uint8_t help);

//sends dummy byte (0xff)
void SendDummyByte(void);

//waits for response on MISO
uint8_t GetResponse(void);

//sends command
uint8_t SendCMD(uint8_t cmd, uint32_t arg, uint8_t crc);

//get additional 32bit of response if needed
uint32_t Get2ndPartResponse(void);

//sends ACMD
uint8_t SendACMD(uint8_t acmd, uint32_t arg, uint8_t crc);

//R/W functions
uint8_t WriteOneBlock(uint8_t address, uint8_t buf[], int size);
uint8_t ReadOneBlock(uint8_t address, uint8_t buf[], int size);
uint8_t WriteMultipleBlocks(uint8_t address, uint8_t buf[], int size, int howMuch);
uint8_t ReadMultipleBlocks(uint8_t from, uint8_t howMuch, uint8_t buf[], int size);
