#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>

#include "IR_tx.h"
#include "Trace.h"

#define MAX_IR_CODE_TIMINGS 128

#define CONTROLLER_ADDRESS 1
#define LIVING_ROOM_ADDRESS 2
#define BEDROOM_ADDRESS 3

// Singleton instance of the radio driver
RH_RF69 driver(8, 3); //cs, irq

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, BEDROOM_ADDRESS);

//
void setup() 
{
	setupIR_TX();

	Serial.begin(115200);
	
	if (!manager.init())
		Serial.println("init failed");
	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM

	if (!driver.setFrequency(915.0))
		Serial.println("setFrequency failed");

	driver.setTxPower(20);
	
	//for (int i = 10; i > 0; i--)
	//{
	//	Serial.println(i);
	//	delay(100);
	//}
	Serial.println("ready");
}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN + 1];

uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
uint8_t *IRptr = (uint8_t *)IRbuf;

//
void logIR(uint16_t *data)
{
	trace("IR_RX_RAW data:\r\n");
	uint16_t *ptr = data;

	uint16_t carrier = *ptr++;

	while (1)
	{
		uint16_t ton = *ptr++;
		uint16_t toff = *ptr++;

		tracef("%4u\t%4u\r\n", ton, toff);

		if (!toff || !ton)
			break;
	}

	uint16_t len = (ptr - data - 1) / 2;
	tracef("IR_RX_RAW data END; carrier:%u len:%u\r\n", carrier, len);
}

//
void logRF(uint8_t *buf, uint8_t len)
{
	for (int i = 0; i < len; i++)
		tracef("%u ", buf[i]);
	trace("\r\n");
}

//
void loop()
{
	if (!manager.available())
		return;
		
	// Wait for a message addressed to us from the client
	uint8_t len = RH_RF69_MAX_MESSAGE_LEN;
	uint8_t from;
	if (manager.recvfromAck(buf, &len, &from))
	{
		//tracef("got request from: %u\r\n", from);
		//logRF(buf, len);

		//IR data
		if (buf[0] == 'd')
		{
			//copy data but skip command character and skip terminating zero
			memcpy(IRptr, buf + 1, len - 2);
			IRptr += (len - 2);
		}
		//send it
		else if (buf[0] == 's')
		{
			logIR(IRbuf);
			sendCode(IRbuf);
			IRptr = (uint8_t *)IRbuf;
		}
		
		// Send a reply back to the originator client
		char reply[] = "ACK";
		if (!manager.sendtoWait((uint8_t *)reply, strlen(reply), from))
			Serial.println("sendtoWait failed");
	}
}
