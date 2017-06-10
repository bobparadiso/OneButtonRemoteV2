#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>

#include "Trace.h"
#include "IR_codes.h"
#include "IR_tx_RF.h"

//MOSI 11
//MISO 12
//SCK 13
RH_RF69 driver(15, 24); //CS, IRQ

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CONTROLLER_ADDRESS);

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN + 1];

uint8_t txAddr = BEDROOM_ADDRESS;

int8_t sendData(uint8_t addr, uint8_t *data, uint8_t txLen);

//
void setTxAddr(uint8_t addr)
{
	txAddr = addr;
}

//
void setupIR_TX_RF()
{
	if (!manager.init())
		Serial.println("setupIR_TX_RF; init failed");
	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	
	if (!driver.setFrequency(915.0))
		Serial.println("setFrequency failed");

	driver.setTxPower(20);
	Serial.println("setupIR_TX_RF; initialized");
}

//
void logRF(uint8_t *buf, uint8_t len)
{
	for (int i = 0; i < len; i++)
		tracef("%u ", buf[i]);
	trace("\r\n");
}

//
int8_t sendData(uint8_t addr, uint8_t *data, uint8_t txLen)
{
	//tracef("sending data to addr: %u\r\n", addr);
	//logRF(data, txLen);
	
	if (manager.sendtoWait(data , txLen, addr))
	{
		// Now wait for a reply from the server
		uint8_t len = RH_RF69_MAX_MESSAGE_LEN;
		uint8_t from;   
		if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
		{
			buf[len] = 0;
			Serial.print(F("got reply from : 0x"));
			Serial.print(from, HEX);
			Serial.print(": ");
			Serial.println((char*)buf);
			return 0;
		}
		else
		{
			Serial.println(F("No reply, is destination running?"));
			return -1;
		}
	}
	else
	{
    	Serial.println(F("sendtoWait failed"));
    	return -1;
    }
}

// 
int8_t sendCode_IR_RF(const char *name)
{
	uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
	memset(IRbuf, 0, sizeof(IRbuf));
	loadIR(name, IRbuf);
	logIR(IRbuf);
	return sendCode_IR_RF(IRbuf);
}

// 
int8_t sendCode_IR_RF(const uint16_t *code)
{
	Serial.println("sendCode\r\n");

	uint8_t txBuf[RH_RF69_MAX_MESSAGE_LEN];
	uint8_t *txPtr = txBuf;
	
	const uint16_t *codePtr = code;
	uint16_t carrier = *codePtr++;

	*txPtr++ = 'd';
	*txPtr++ = carrier & 0xff;
	*txPtr++ = carrier >> 8;

	while (1)
	{
		uint16_t on = *codePtr++;
		uint16_t off = *codePtr++;

		#define TIMING_SIZE 4
		*txPtr++ = on & 0xff;
		*txPtr++ = on >> 8;
		*txPtr++ = off & 0xff;
		*txPtr++ = off >> 8;
		
		uint8_t txLen = txPtr - txBuf;
		
		if (off == 0)
		{
			*txPtr++ = 0; //sending extra end byte since receiver will sometimes zero out last byte
			if (sendData(txAddr, txBuf, txLen + 1) != 0)
				return -1;
			txPtr = txBuf + 1; //start filling next buf
			break;
		}

		if (txLen > RH_RF69_MAX_MESSAGE_LEN - (TIMING_SIZE + 1))
		{
			*txPtr++ = 0; //sending extra end byte since receiver will sometimes zero out last byte
			if (sendData(txAddr, txBuf, txLen + 1) != 0)
				return -1;
			txPtr = txBuf + 1; //start filling next buf
		}
	}

	txBuf[0] = 's';
	txBuf[1] = 0; //sending extra end byte since receiver will sometimes zero out last byte
	if (sendData(txAddr, txBuf, 2) != 0)
		return -1;
	
	return 0;
}
