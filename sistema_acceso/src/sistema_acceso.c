/* Copyright 2019, MMR - Project: Sistema de acceso.
 * Mamani Brian - Mamani Facundo - Ramos Isaac
 * All rights reserved.
 *
 * This file is not part sAPI library for microcontrollers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Date: 2019-08-01
 */

/*==================[inclusions]=============================================*/

#include "sistema_acceso.h"   // <= own header (optional)#include "PN532.h"#include "sd_spi.h"   // <= own header (optional)#include "sapi.h"     // <= sAPI header#include "ff.h"       // <= Biblioteca FAT FS/*==================[macros and definitions]=================================*/
#define FILENAME "hola.txt"
#define CLAVES_VALIDAS "claves.txt"

#define UART_SELECTED   UART_USB

typedef enum {
	ESTADO_PERMITIDO,
	ESTADO_INCORRECTO,
	ESTADO_INICIAL,
	ESTADO_BIENVENIDO,
	ESTADO_CLAVE,
	ESTADO_MENU,
	ALTA_LLAVE,
	BAJA_LLAVE
} estadoMEF;
/*==================[internal data declaration]==============================*/

static FATFS fs;           // <-- FatFs work area needed for each volume
static FIL fp;             // <-- File object needed for each open file

uint8_t mensaje[1024];
uint8_t contenidoSD[1024];
uint8_t clave[14];
//uint8_t nombre[]={"MAMANI FACUNDO"};
uint8_t texto[1024];
bool_t banderaBaja=0, banderaAlta=0, banderaParpadeo=1, banderaSiguiente=1;



CONSOLE_PRINT_ENABLE

uint8_t cantidadLlavesValidas;
#define LONGITUD_LLAVE 			8

/*==================[internal functions declaration]=========================*/
/*==================[internal data definition]===============================*/

CONSOLE_PRINT_ENABLE
estadoMEF estadoActual;

/*==================[external data definition]===============================*/

static char uartBuff[10];

/*==================[internal functions definition]==========================*/
void clear() {
	uartWriteByte(UART_USB, 27);
	uartWriteString(UART_USB, "[2J");
	uartWriteByte(UART_USB, 27);
	uartWriteString(UART_USB, "[H");
}
void InicializarMEF(estadoMEF *punteroEstado);
void ActualizarMEF(estadoMEF *punteroEstado);
uint8_t teclaMatricial(uint16_t tecla);

uint8_t llaveValida = 0;

/*==================[external functions definition]==========================*/

void diskTickHook(void *ptr);

bool_t compararllaves(uint8_t llaveIngresada[], uint8_t llave[][LONGITUD_LLAVE],
		uint8_t longitud) {
	uint8_t i;
	for (llaveValida = 0; llaveValida < cantidadLlavesValidas; llaveValida++) {
		for (i = 0; llaveIngresada[i] == llave[llaveValida][i] && i < longitud;
				i++)
			;
		if (i == longitud) {
			return TRUE;
		}
	}
	return FALSE;
}

/*------------------------------------------------------------------------------------------*/

char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) {
		*result = '\0';
		return result;
	}

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ =
				"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35
						+ (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

/*---------------------------------------------------------------------------------------*/
bool_t posiciones(uint8_t filas[], uint8_t columnas[]) {

}

/*---------------------------------------------------------------------------------------*/
/* Enviar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
void showDateAndTime(rtc_t * rtc) {
	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->mday), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio el dia */
	if ((rtc->mday) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(uartBuff);
	lcdSendStringRaw("/");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->month), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio el mes */
	if ((rtc->month) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(uartBuff);
	lcdSendStringRaw("/");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->year), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio el aÃ±o */
	if ((rtc->year) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(uartBuff);
	lcdSendStringRaw("  ");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->hour), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio la hora */
	if ((rtc->hour) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(uartBuff);
	lcdSendStringRaw(":");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->min), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio los minutos */
	// uartBuff[2] = 0;    /* NULL */
	if ((rtc->min) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(uartBuff);

	//lcdSendStringRaw(":");

	/*Conversion de entero a ascii con base decimal */
	//itoa((int) (rtc->sec), (char*) uartBuff, 10); /* 10 significa decimal */
	/* Envio los segundos */
	//if ((rtc->sec) < 10)
	//lcdSendStringRaw("0");
	//lcdSendStringRaw(uartBuff);
	/* Envio un 'enter' */
	//uartWriteString(UART_USB, "\r\n");
}

/*-------------------------------------------------------------------------------------------*/
/* FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE RESET. */

uint16_t miliSegundos = 0;
uint16_t segundos = 0;
uint8_t uid[20], uidLength, entrada, claveGuardada[4] = { '1', '2', '3', '4' },
		claveIngresada[4];
uint8_t  banderaClaveIncorrecta, opcion;
uint16_t i, j;

int16_t clavesEncontradas;

// [columnas][filas]

/*
uint8_t clavesValidas[20][LONGITUD_LLAVE] = { { 0x8D, 0x41, 0x4D, 0x79 }, {
		0xef, 0x07, 0x25, 0x3c }, };

*/

uint8_t clavesValidas[100][LONGITUD_LLAVE];

uint8_t apellido[100][16], nombre[100][16], letra;

/*------------------------------------------------------------------*/

/*uint8_t datoRecibido[8] = { T_COL0, // Segment 'a'
 T_FIL2, // Segment 'b'
 T_FIL3, // Segment 'c'
 T_FIL0, // Segment 'd'
 T_COL1, // Segment 'e'
 CAN_TD, // Segment 'f'
 CAN_RD, // Segment 'g'
 T_FIL1  // Segment 'h' or 'dp'
 };*/

/* Configuracion de pines para el Teclado Matricial*/

/*
 // Teclado
 keypad_t keypad;

 // Filas --> Salidas
 uint8_t keypadRowPins1[4] = { T_FIL1, // Row 0
 CAN_RD,    // Row 1
 CAN_TD,    // Row 2
 T_COL1     // Row 3
 };

 // Columnas --> Entradas con pull-up (MODO = GPIO_INPUT_PULLUP)
 uint8_t keypadColPins1[4] = { T_FIL0,    // Column 0
 T_FIL3,    // Column 1
 T_FIL2,    // Column 2
 T_COL0     // Column 3
 };

 keypadConfig(&keypad, keypadRowPins1, 4, keypadColPins1, 4);

 // Variable para guardar la tecla leida
 uint16_t tecla = 0;
 */

// Teclado
keypad_t keypad;

// Variable para guardar la tecla leida
uint16_t tecla = 0;

delay_t espera;
delay_t parpadeo;

/*------------------------------------------------------------------*/
int main(void) {

	/* ------------- INICIALIZACIONES ------------- */

	/* Inicializar la placa */
	boardConfig();

	uartConfig(UART_USB, 115200);

	uartWriteString(UART_USB, "Prueba SD ...");

	// SPI configuration
	spiConfig(SPI0);

	// Inicializar el conteo de Ticks con resolucion de 10ms,
	// con tickHook diskTickHook
	//tickConfig(10);
	//tickCallbackSet(diskTickHook, NULL);



	delayConfig(&espera, 1000);
	delayConfig(&parpadeo, 300);

	// Filas --> Salidas
	uint8_t keypadRowPins1[4] = { T_FIL1, // Row 0
			CAN_RD,    // Row 1
			CAN_TD,    // Row 2
			T_COL1     // Row 3
			};

	// Columnas --> Entradas con pull-up (MODO = GPIO_INPUT_PULLUP)
	uint8_t keypadColPins1[4] = { T_FIL0,    // Column 0
			T_FIL3,    // Column 1
			T_FIL2,    // Column 2
			T_COL0     // Column 3
			};

	keypadConfig(&keypad, keypadRowPins1, 4, keypadColPins1, 4);

	/* Inicializar UART_USB a 115200 baudios */

	gpioConfig(GPIO8, GPIO_OUTPUT);
	consolePrintConfigUart(UART_USB, 115200);

	lcdInit(16, 2, 5, 8);

	lcdCommand( 0x0C );                   // Command 0x0E for display on, cursor on
	lcdCommandDelay();                    // Wait

	estadoMEF estadoActual;

	InicializarMEF(&estadoActual);

	while ( TRUE) {
		ActualizarMEF(&estadoActual);
		delay(1);
		miliSegundos++;
	}
	return 0;
}
/* ------------- REPETIR POR SIEMPRE ------------- */
void InicializarMEF(estadoMEF *punteroEstado) {
	*punteroEstado = ESTADO_INICIAL;
	miliSegundos = 0;
}

void ActualizarMEF(estadoMEF *punteroEstado) {
	while (1) {
		switch (*punteroEstado) {

		case ESTADO_INICIAL:

			/* Estructura RTC */

			while (!pn532Config(UART_232, 115200)) {
				clear();
				lcdClear();
				lcdGoToXY(3, 1);
				lcdSendStringRaw("Error");
				lcdGoToXY(3, 2);
				lcdSendStringRaw("configurando");
			}

			clear();
			lcdClear();
			lcdGoToXY(1, 1);
			lcdSendStringRaw("TARJETA SD:");
			lcdGoToXY(1, 2);
			lcdSendStringRaw("leyendo...");

			gpioWrite(GPIO4, ON);

			delay_t delay1s;
			delayConfig(&delay1s, 1000);

			/* Estructura RTC */
			rtc_t rtc;

			/*rtc.year = 19;
			 rtc.month = 10;
			 rtc.mday = 24;
			 rtc.wday = 4; // 1 lunes
			 rtc.hour = 9;
			 rtc.min = 22;
			 rtc.sec = 0;*/

			bool_t val = 0;

			// COMIENZO PRUEBA SD

			UINT nbytes;

			// Give a work area to the default drive
			if (f_mount(&fs, "", 0) != FR_OK) {
				// If this fails, it means that the function could
				// not register a file system object.
				// Check whether the SD card is correctly connected
			}

			// Create/open a file, then write a string and close it



			for (i = 0; i < 5; i++) {

				if (f_open(&fp, FILENAME, FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
					f_write(&fp, "Hola mundo\r\n", 12, &nbytes);

					f_close(&fp);

					if (nbytes == 12) {
						// Turn ON LEDG if the write operation was successful
						gpioWrite(LEDG, ON);
					}
				} else {
					// Turn ON LEDR if the write operation was fail
					gpioWrite(LEDR, ON);
				}
			}

			for (i = 0; i < 5; i++) {

				if (f_open(&fp, FILENAME, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
					f_read(&fp, mensaje, 1024, &nbytes);

					f_close(&fp);

					if (nbytes == 1024) {
						// Turn ON LEDG if the write operation was successful
						gpioWrite(LEDG, ON);
					}
				} else {
					// Turn ON LEDR if the write operation was fail
					gpioWrite(LEDR, ON);
				}
			}

			uartWriteString(UART_USB, mensaje);

			if (f_open(&fp, CLAVES_VALIDAS, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
				f_read(&fp, contenidoSD, 1024, &nbytes);

				f_close(&fp);
			}

			i=0;

			for(clavesEncontradas=0;contenidoSD[i]!='\0';clavesEncontradas++){
				j=0;
				while(contenidoSD[i]!=',' ){
					apellido[clavesEncontradas][j]=contenidoSD[i];

					i++;
					j++;
					if(i==1024){
						goto sinClaves;
					}
				}

				apellido[clavesEncontradas][j]='\0';

				i=i+2;

				j=0;

				while(contenidoSD[i]!=';' ){
					nombre[clavesEncontradas][j]=contenidoSD[i];

					i++;
					j++;

				}

				nombre[clavesEncontradas][j]='\0';

				if(contenidoSD[i+1]==' '){

					i=i+2; // me paro en el primer numero de la clave

					for(j=0;contenidoSD[i+j]!='\r';j++){
						clavesValidas[clavesEncontradas][j]=contenidoSD[i+j];
					}



					i=i+j+2; // me paro despues del ultimo numero de la clave
				}
				else{
					clavesEncontradas--;
				}
			}

			cantidadLlavesValidas = clavesEncontradas;

			if(i < 1024 && i != 0){

			lcdClear();
			lcdGoToXY(1, 1);
			lcdSendStringRaw("Tarjeta SD:");
			lcdGoToXY(1, 2);
			lcdSendStringRaw("lectura OK !!!");
			}
			else
			{
				sinClaves:

				lcdClear();
				lcdGoToXY(1, 1);
				lcdSendStringRaw("Tarjeta SD:");
				lcdGoToXY(1, 2);
				lcdSendStringRaw("SIN CLAVES !");
			}


			delay(2000);

			/*

			if (f_open(&fp, CLAVES_VALIDAS, FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
				f_write(&fp, "Hola mundo\r\n", 12, &nbytes);

				f_close(&fp);
			}

			*/
			// FIN PRUEBA SD

			*punteroEstado = ESTADO_BIENVENIDO;

			/* Inicializar RTC */
			//val = rtcConfig(&rtc);
			break;

		case ESTADO_BIENVENIDO:
			if (delayRead(&delay1s)) {
				lcdClear();
				lcdGoToXY(4, 1);
				lcdSendStringRaw("Bienvenido!");
				lcdGoToXY(1, 2);

				val = rtcRead(&rtc);
				/* Mostrar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
				showDateAndTime(&rtc);
				gpioWrite(GPIO8, OFF);
			}

			if (pn532ReadPassiveTargetID(uid, &uidLength)) {

				/*

				//intToHexString(uid,uidLength,clave);

				texto[0]='\0';

				//strcat(texto,nombre);
				//strcat(texto,", ");
				strcat(texto,"; ");
				strcat(texto,uid);

				texto[48]='\r';
				texto[49]='\n';

				if (f_open(&fp, CLAVES_VALIDAS, FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
					f_write(&fp, texto, sizeof(texto), &nbytes);

					f_close(&fp);
				}

				if (f_open(&fp, CLAVES_VALIDAS, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
					f_read(&fp, mensaje, 1024, &nbytes);

					f_close(&fp);
				}

				uartWriteString(UART_USB, mensaje);

				printHexBuf(uid, uidLength);

				//intToHexString(mensaje,uidLength,clave);

				//uartWriteString(UART_USB, clave);

				*/

				// Abrís el log
				// escribís fecha y hora
				// escribís tarjeta
				if (compararllaves(uid, clavesValidas, uidLength) == TRUE) {
					// escribís "Permitido"
					*punteroEstado = ESTADO_PERMITIDO;

				} else {
					// escribís "Denegado"
					*punteroEstado = ESTADO_INCORRECTO;
				}
				// Cerrás el log
			}

			if (keypadRead(&keypad, &tecla)) {

				*punteroEstado = ESTADO_CLAVE;

				while (keypadRead(&keypad, &tecla))
					;
			}
			break;

		case ESTADO_PERMITIDO:
			lcdClear();
			lcdGoToXY(4, 1);
			lcdSendStringRaw("Acceso");
			lcdGoToXY(4, 2);
			lcdSendStringRaw("permitido");
			val = rtcRead(&rtc);
			gpioWrite(GPIO8, ON);
			delay(2000);
			*punteroEstado = ESTADO_BIENVENIDO;
			break;

		case ESTADO_INCORRECTO:
			lcdClear();
			lcdGoToXY(4, 1);
			lcdSendStringRaw("Tarjeta");
			lcdGoToXY(4, 2);
			lcdSendStringRaw("incorrecta");
			gpioWrite(GPIO8, OFF);
			delay(2000);
			*punteroEstado = ESTADO_BIENVENIDO;
			break;

		case ESTADO_CLAVE:
			banderaClaveIncorrecta = 0;
			lcdClear();
			lcdGoToXY(2, 1);
			lcdSendStringRaw("Ingrese clave");
			lcdGoToXY(1, 2);

			// Ingreso de clave

			for (i = 0; i <= 3; i++) {
				segundos = 0;
				while (keypadRead(&keypad, &tecla) == 0) { // espero a que se presione una tecla + timeOut
					if (delayRead(&espera)) {
						segundos++;

						if (segundos == 5) {
							*punteroEstado = ESTADO_BIENVENIDO;
							goto salir;
						}
					}
				}

				lcdSendStringRaw("*");
				delay(100);
				while (keypadRead(&keypad, &tecla) == 1)
					;
				claveIngresada[i] = teclaMatricial(tecla);

			}

			// Comparar claves

			for (j = 0; j < 4; j++) {

				if (claveIngresada[j] != claveGuardada[j]) {
					banderaClaveIncorrecta = 1;
				}
			}

			if (banderaClaveIncorrecta == 1) {
				lcdClear();
				lcdGoToXY(1, 1);
				lcdSendStringRaw("Clave incorrecta");
				delay(2000);

				*punteroEstado = ESTADO_BIENVENIDO;

			} else {
				lcdClear();
				lcdGoToXY(1, 1);
				lcdSendStringRaw("Clave correcta!");
				delay(2000);
				*punteroEstado = ESTADO_MENU;
				opcion = 0;
			}
			salir:

			break;

		case ESTADO_MENU:
			lcdClear();
			lcdGoToXY(2, 1);
			lcdSendStringRaw("Menu principal");
			lcdGoToXY(2, 2);
			//delay(2000);

			switch (opcion) {
			case 0:

				lcdSendStringRaw("<Alta llave>");
				lcdGoToXY(1, 2);
				break;

			case 1:

				lcdSendStringRaw("<Baja llave>");

				lcdGoToXY(1, 2);
				break;

			case 2:
				lcdSendStringRaw("<Salir>");
				lcdGoToXY(1, 2);
				break;
			}

			while (keypadRead(&keypad, &tecla) == 0)
				;

			delay(100);

			while (keypadRead(&keypad, &tecla) == 1)
				;

			tecla = teclaMatricial(tecla);

			switch (tecla) {
			case '*':
				opcion--;
				if (opcion == 255) {
					opcion = 2;
				}
				break;
			case '#':
				opcion++;
				if (opcion == 3) {
					opcion = 0;
				}
				break;
			case '0':
				switch (opcion) {
				case 0:

					*punteroEstado = ALTA_LLAVE;

					break;
				case 1:

					*punteroEstado = BAJA_LLAVE;

					break;
				case 2:
					*punteroEstado = ESTADO_BIENVENIDO;
					break;
				}
				break;
			}

			break;

		case ALTA_LLAVE:

			lcdClear();
			lcdSendStringRaw("Ingrese apellido:");
			lcdGoToXY(1, 2);

			letra='A';
			i=0;

			do{


				while (keypadRead(&keypad, &tecla) == 0){
					if(delayRead(&parpadeo)){
						if(banderaParpadeo==1){
							banderaParpadeo=0;
							lcdData(letra);
							lcdCommand(0x10);
						}else{
							banderaParpadeo=1;
							lcdData(' ');
							lcdCommand(0x10);
						}
					}
				}

				if (keypadRead(&keypad, &tecla) == 1){
					lcdData(letra);
					lcdCommand(0x10);
				}

				delay(150);
				tecla = teclaMatricial(tecla);

				switch (tecla) {
				case '*':
					letra--;

					if (letra == 64) {
						letra = 90;
					}
					break;
				case '#':
					letra++;

					if (letra == 91) {
						letra = 65;
					}
					break;
				case '0':

					while (keypadRead(&keypad, &tecla) == 1);

					apellido[cantidadLlavesValidas][i]=letra;
					i++;

					if(i==15){
					  tecla='D';
					}

					letra='A';

					lcdCommand(0x14);

					break;
				case 'C':
					if(i>0)i--;

					lcdData(' ');
					lcdCommand(0x10);
					lcdCommand(0x10);

					break;
				}


			}while(tecla!='D');

			while (keypadRead(&keypad, &tecla) == 1);

			apellido[cantidadLlavesValidas][i]='\0';

/*---------------------------------------------------------*/

			lcdClear();
			lcdSendStringRaw("Ingrese nombre:");
			lcdGoToXY(1, 2);

			letra = 'A';
			i = 0;

			do {
				while (keypadRead(&keypad, &tecla) == 0){
					if(delayRead(&parpadeo)){
						if(banderaParpadeo==1){
							banderaParpadeo=0;
							lcdData(letra);
							lcdCommand(0x10);
						}else{
							banderaParpadeo=1;
							lcdData(' ');
							lcdCommand(0x10);
						}
					}
				}

				if (keypadRead(&keypad, &tecla) == 1){
					lcdData(letra);
					lcdCommand(0x10);
				}

				delay(150);
				tecla = teclaMatricial(tecla);

				switch (tecla) {
				case '*':
					letra--;

					if (letra == 64) {
						letra = 90;
					}
					break;
				case '#':
					letra++;

					if (letra == 91) {
						letra = 65;
					}
					break;
				case '0':

					nombre[cantidadLlavesValidas][i] = letra;
					i++;

					if (i == 15) {
						tecla = 'D';
					}

					letra = 'A';

					lcdCommand(0x14);

					break;
				case 'C':
					if(i>0)i--;

					lcdData(' ');
					lcdCommand(0x10);
					lcdCommand(0x10);

					break;
				}

			} while (tecla != 'D');

			nombre[cantidadLlavesValidas][i] = '\0';

			lcdClear();
			lcdSendStringRaw("Acerque la");
			lcdGoToXY(1, 2);
			lcdSendStringRaw("tarjeta...");
			//lcdGoToXY(1, 2);

			//do {
			while (pn532ReadPassiveTargetID(uid, &uidLength) == FALSE);

			//} while (uid[0] == 0);

			if (compararllaves(uid, clavesValidas, uidLength) == TRUE) {

				lcdClear();
				lcdSendStringRaw("La tarjeta ya");
				lcdGoToXY(1, 2);
				lcdSendStringRaw("estaba de alta!");
				delay(2000);
				*punteroEstado = ESTADO_MENU;

			} else {

				cantidadLlavesValidas++;

				for (i = 0; i < uidLength; i++) {
					clavesValidas[cantidadLlavesValidas - 1][i] = uid[i];
				}

				texto[0]='\0';

				for(i=0;i<cantidadLlavesValidas;i++){

					// copia la clave a uid


					for(j=0;j<uidLength;j++){
						uid[j] = clavesValidas[i][j];
					}

					strcat(texto,&apellido[i][0]);
					strcat(texto,", ");
					strcat(texto,&nombre[i][0]);
					strcat(texto,"; ");
					strcat(texto,uid);
					strcat(texto,"\r\n");
				}

				banderaAlta=0;

				if (f_open(&fp, CLAVES_VALIDAS, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {

					f_write(&fp, texto, strlen(texto), &nbytes);

					f_close(&fp);

					banderaAlta=1;
				}

				//intToHexString(mensaje,uidLength,clave);

				//uartWriteString(UART_USB, clave);

				if(banderaAlta==1){

				lcdClear();
				lcdSendStringRaw("Llave cargada!");
				delay(2000);
				}
				else{
					lcdClear();
					lcdSendStringRaw("Error al");
					lcdGoToXY(1, 2);
					lcdSendStringRaw("escribir SD!");
					delay(2000);
				}
				*punteroEstado = ESTADO_MENU;

			}

			break;

		case BAJA_LLAVE:

			lcdClear();
			lcdSendStringRaw("Dar de baja:");

			i=0;

			do{

				while(apellido[i][0]=='\0'){
					if(banderaSiguiente==1)
					{
						i++;
						if(i==cantidadLlavesValidas)
						{
							i=0;
						}
					}
					else
					{
						i--;
						if(i==65535)
						{
							i=cantidadLlavesValidas-1;
						}
					}
				}

				lcdGoToXY(1, 2);
				lcdSendStringRaw(&apellido[i][0]);
				lcdSendStringRaw(", ");
				lcdData(nombre[i][0]);
				lcdData('.');

				while (keypadRead(&keypad, &tecla) == 0);
				delay(150);
				tecla = teclaMatricial(tecla);

				switch (tecla) {
				case '*':
					i--;
					banderaSiguiente=0;
					if(i==65535)
					{
						i=cantidadLlavesValidas-1;
					}


					break;
				case '#':
					i++;
					banderaSiguiente=1;
					if(i==cantidadLlavesValidas)
					{
						i=0;
					}

					break;
				case '0':

					lcdClear();
					lcdSendStringRaw("Estas seguro?");
					lcdGoToXY(1, 2);
					lcdSendStringRaw("D: Si  ;  C: No");

					while (keypadRead(&keypad, &tecla) == 0);
					delay(150);
					tecla = teclaMatricial(tecla);

					break;
				}

			}while(tecla!='D'&&tecla!='C');

			if(tecla=='D'){

				for(j=0;j<16;j++){
					apellido[i][j]='\0';
					nombre[i][j]='\0';

					if(j>=0&&j<LONGITUD_LLAVE){
						clavesValidas[i][j]=0;
					}
				}

				//cantidadLlavesValidas--;

				texto[0]='\0';

				for(i=0;i<cantidadLlavesValidas;i++){


					if(apellido[i][0]!='\0'){

						for(j=0;j<LONGITUD_LLAVE;j++){
							uid[j] = clavesValidas[i][j];
						}

						strcat(texto,&apellido[i][0]);
						strcat(texto,", ");
						strcat(texto,&nombre[i][0]);
						strcat(texto,"; ");
						strcat(texto,uid);
						strcat(texto,"\r\n");

					}
				}

				banderaBaja=0;

				if (f_open(&fp, CLAVES_VALIDAS, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {

					f_write(&fp, texto, strlen(texto), &nbytes);

					f_close(&fp);

					banderaBaja=1;
				}

				if(banderaBaja==1){

					lcdClear();
					lcdSendStringRaw("Llave dada");
					lcdGoToXY(1, 2);
					lcdSendStringRaw("de baja!");
					delay(2000);

				}
				else{
					lcdClear();
					lcdSendStringRaw("Error al");
					lcdGoToXY(1, 2);
					lcdSendStringRaw("escribir SD!");
					delay(2000);
				}

			}

			*punteroEstado = ESTADO_MENU;

			break;

		}

	}
}


uint8_t teclaMatricial(uint16_t tecla) {

	switch (tecla) {
	case 0:
		return '1';
		break;
	case 1:
		return '2';
		break;
	case 2:
		return '3';
		break;
	case 3:
		return 'A';
		break;
	case 4:
		return '4';
		break;
	case 5:
		return '5';
		break;
	case 6:
		return '6';
		break;
	case 7:
		return 'B';
		break;
	case 8:
		return '7';
		break;
	case 9:
		return '8';
		break;
	case 10:
		return '9';
		break;
	case 11:
		return 'C';
		break;
	case 12:
		return '*';
		break;
	case 13:
		return '0';
		break;
	case 14:
		return '#';
		break;
	case 15:
		return 'D';
		break;
	}
}

void intToHexString(uint8_t buf[], uint8_t len_buf, uint8_t clave[]) {
	uint8_t i, num_max, num_min;
	for(i = 0; i < len_buf; i++) {
		num_max = ((buf[i]>>4)&0x0F);
		num_min = (buf[i]&0x0F);
		if(num_max >= 0 && num_max <= 9)		num_max += 48;
		else									num_max += 55;
		if(num_min >= 0 && num_min <= 9)		num_min += 48;
		else									num_min += 55;
		clave[i*3] =  num_max;
		clave[(i*3)+1] =  num_min;
		clave[(i*3)+2] =  ' ';
	}

	//clave[12]='\r';
	//clave[13]='\n';
}

void diskTickHook(void *ptr) {
	disk_timerproc();   // Disk timer process
	return 1;
}

/*==================[end of file]============================================*/
