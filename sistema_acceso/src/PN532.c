/* Copyright 2019, Nicolás Potenza.
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

#include "sapi.h"     // <= sAPI header
#include "PN532.h"

/*==================[macros and definitions]=================================*/
//#define DEBUG_SEND
//#define DEBUG_RECV

#ifdef DEBUG_SEND
#define PN532_SEND(buf)		uartWriteByte(_uart, (buf));	uartWriteByte(UART_USB, (buf))
#else
#define PN532_SEND(buf)		uartWriteByte(_uart, (buf))
#endif

#ifdef DEBUG_RECV
#define PN532_RECV(buf)		uartReadByte(_uart, (buf));		uartWriteByte(UART_USB, (*buf))
#else
#define PN532_RECV(buf)		uartReadByte(_uart, (buf))
#endif

/*==================[internal data declaration]==============================*/
uartMap_t _uart;
uint32_t _baudRate;
/*==================[internal functions declaration]=========================*/
void pn532SendCommand(uint8_t buf[], uint8_t len);
void pn532SendACK();
void pn532SendNACK();
void pn532SendError();

void pn532RecvResponse(uint8_t buf[], uint8_t len_buf);
void pn532RecvClean();

void pn532WakeUp();
void pn532SamConfiguration();
void pn532RFConfiguration();
/*==================[internal data definition]===============================*/
uint8_t pn532ACK[]	= { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
uint8_t pn532NACK[]	= { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/
void pn532SendCommand(uint8_t buf[], uint8_t len) {
	uint8_t i, lcs, dcs, bufsum = 0;
	PN532_SEND(0x00); // Preamble
	PN532_SEND(0x00); PN532_SEND(0xFF); // Start Code

	PN532_SEND(len+1); // length
	lcs = - (len+1);
	PN532_SEND(lcs); // length checksum

	PN532_SEND(0xD4); // TFI send
	for(i = 0; i < len; i++) {
		PN532_SEND(buf[i]); // data
		bufsum += buf[i];
	}
	dcs = - (0xD4 + bufsum);
	PN532_SEND(dcs); // data checksum

	PN532_SEND(0x00); // Postamble
}

void pn532SendACK() {
	PN532_SEND(0x00); // Preamble
	PN532_SEND(0x00); PN532_SEND(0xFF); // Start Code
	PN532_SEND(0x00); PN532_SEND(0xFF); // ACK Code
	PN532_SEND(0x00); // Postamble
}

void pn532SendNACK() {
	PN532_SEND(0x00); // Preamble
	PN532_SEND(0x00); PN532_SEND(0xFF); // Start Code
	PN532_SEND(0xFF); PN532_SEND(0x00); // NACK Code
	PN532_SEND(0x00); // Postamble
}

void pn532SendError() {
	PN532_SEND(0x00); // Preamble
	PN532_SEND(0x00); PN532_SEND(0xFF); // Start Code
	PN532_SEND(0x01); // length
	PN532_SEND(0xFF); // length Checksum
	PN532_SEND(0x7F); // specific aplication level error Code
	PN532_SEND(0x81); // data checksum
	PN532_SEND(0x00); // Postamble
}

void pn532RecvResponse(uint8_t buf[], uint8_t len_buf) {
	uint8_t i = 0;
	for( i = 0; i < len_buf; i++ ) {
		while(!uartRxReady(_uart));
		PN532_RECV(&buf[i]);
	}
}

void pn532RecvClean() {
	uint8_t buf;
	while(uartReadByte(_uart, &buf));
}

void pn532WakeUp() {
	PN532_SEND(0x55);
	PN532_SEND(0x55);
	PN532_SEND(0x00);
	PN532_SEND(0x00);
}

void pn532SamConfiguration() {
	uint8_t buf[] = { 0x14, 0x01, 0x14, 0x01 };
	pn532SendCommand(buf, sizeof(buf));
}

void pn532RFConfiguration() {
	uint8_t buf[] = { 0x32, 0x05, 0xFF, 0x01, 0xFF };
	pn532SendCommand(buf, sizeof(buf));
}

bool_t pn532Search(const uint8_t buf[], uint8_t len_buf, const uint8_t search[], uint8_t len_search) {
	uint8_t index = 0, i = 0;
	bool_t ret = FALSE;

	while(index < len_buf) {
		i = 0;
		while(buf[index+i] == search[i] && i < len_search)
			i++;
		if(i == len_search)
			return TRUE;
		else
			index++;
	}
	return ret;
}

/*==================[external functions definition]==========================*/
bool_t pn532Config( uartMap_t uart, uint32_t baudRate )
{
	_uart = uart;
	_baudRate = baudRate;

	uint8_t buf[255], response[255];

	uartConfig(_uart, _baudRate);
	pn532WakeUp();

	pn532GetFirmwareVersion();
	pn532RecvResponse(buf, 6);
	if (pn532Search(buf, 6, pn532ACK, 6)) {
		response[0] = 0x03;
		pn532RecvResponse(buf, 13);
		if(pn532Search(buf, 13, response, 1))
		{
			pn532SendACK();
//			gpioWrite(LED1, ON);
		}else return FALSE;
	}else return FALSE;
	pn532RecvClean();

	pn532RFConfiguration();
	pn532RecvResponse(buf, 6);
	if (pn532Search(buf, 6, pn532ACK, 6)) {
		response[0] = 0xD5;
		response[1] = 0x33;
		pn532RecvResponse(buf, 9);
		if(pn532Search(buf, 9, response, 2))
		{
			pn532SendACK();
//			gpioWrite(LED2, ON);
		}else return FALSE;
	}else return FALSE;
	pn532RecvClean();

	pn532SamConfiguration();
	pn532RecvResponse(buf, 6);
	if (pn532Search(buf, 6, pn532ACK, 6)) {
		response[0] = 0xD5;
		response[1] = 0x15;
		pn532RecvResponse(buf, 9);
		if(pn532Search(buf, 9, response, 2))
		{
			pn532SendACK();
//			gpioWrite(LED3, ON);
		}else return FALSE;
	}else return FALSE;
	pn532RecvClean();

//	gpioWrite(LED1, OFF);
//	gpioWrite(LED2, OFF);
//	gpioWrite(LED3, OFF);
	return TRUE;
}


void pn532GetFirmwareVersion() {
	uint8_t buf[] = { 0x02 };
	pn532SendCommand(buf, sizeof(buf));
}


bool_t pn532ReadPassiveTargetID(uint8_t uid[], uint8_t *uidLength) {

	uint8_t response[20];
	uint8_t i;
	uint8_t buf[] = { 0x4A , 0x01 , 0x00 };
	uint8_t ret[20];

	pn532RecvClean();
	pn532SendCommand(buf, sizeof(buf));
	pn532RecvResponse(ret, 6);
	if (pn532Search(ret, 6, pn532ACK, 6)) {
		response[0] = 0x4B;
		response[1] = 0x01;

//		gpioWrite(LEDG, ON);

		for( i = 0; i < 50 && !uartRxReady(_uart); i++ )	delay(1);
		if (uartRxReady(_uart)) {
			pn532RecvResponse(ret, 19);
			if(pn532Search(ret, 19, response, 2))
			{
				pn532SendACK();
				*uidLength = ret[12];
				for(i = 0; i < ret[12]; i++)
					uid[i] = ret[13+i];
//				gpioWrite(LEDB, ON);
			}else return FALSE;
		}else return FALSE;
	}else return FALSE;
	return TRUE;
}

void printHexBuf(uint8_t buf[], uint8_t len_buf) {
	uint8_t i, num_max, num_min;
	for(i = 0; i < len_buf; i++) {
		num_max = ((buf[i]>>4)&0x0F);
		num_min = (buf[i]&0x0F);
		if(num_max >= 0 && num_max <= 9)		num_max += 48;
		else									num_max += 55;
		if(num_min >= 0 && num_min <= 9)		num_min += 48;
		else									num_min += 55;
		uartWriteByte(UART_USB, num_max);
		uartWriteByte(UART_USB, num_min);
		uartWriteByte(UART_USB, ' ');
	}
}
/*==================[end of file]============================================*/
