#include <Arduino.h>
#include "Trace.h"
#include "IR_codes.h"
#include "IR_tx.h"

#define IR_LED_PIN 20

//
void setupIR_TX()
{
	pinMode(IR_LED_PIN, OUTPUT);
}

// This procedure sends a given kHz burst to the IR LED pin 
// for a certain # of microseconds. We'll use this whenever we need to send codes
void sendBurst(uint16_t carrier, long microsecs)
{
	uint16_t waveLength = 1000000 / carrier;
	uint16_t halfWaveLength = waveLength / 2;

	//tracef("sendBurst; carrier:%u waveLength:%u microsecs:%u\r\n", carrier, waveLength, microsecs);
	cli();  // this turns off any background interrupts

	while (microsecs > 0)
	{
		digitalWriteFast(IR_LED_PIN, 1);
		delayMicroseconds(waveLength - halfWaveLength); //for odd wavelengths
		digitalWriteFast(IR_LED_PIN, 0);
		delayMicroseconds(halfWaveLength);

		microsecs -= waveLength;
	}

	sei();  // this turns them back on
}

// 
int8_t sendCode_IR(const char *name)
{
	uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
	memset(IRbuf, 0, sizeof(IRbuf));
	loadIR(name, IRbuf);
	logIR(IRbuf);
	return sendCode_IR(IRbuf);
}

//
int8_t sendCode_IR(const uint16_t *code)
{
	Serial.println("sendCode\r\n");

	const uint16_t *ptr = code;
	uint16_t carrier = *ptr++;
	while (1)
	{
		int on = *ptr++;
		int off = *ptr++;
		if (on) //check if there's a burst to send or if this is the continuation of the last SPACE
			sendBurst(carrier, on);
		delayMicroseconds(off);
		if (!off) //a SPACE of 0 indicates finished
			return 0;
	}
}
