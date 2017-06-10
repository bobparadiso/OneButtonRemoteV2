#include <Arduino.h>
#include "Trace.h"
#include "IR_tx.h"

#define IR_LED_PIN 12

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
	noInterrupts();

#define REGTYPE uint32_t
	REGTYPE IRpin;
	volatile REGTYPE *outIRpin;

	IRpin = digitalPinToBitMask(IR_LED_PIN);
	outIRpin = portOutputRegister(digitalPinToPort(IR_LED_PIN));
  	
	while (microsecs > 0)
	{
		*outIRpin |= IRpin;
		delayMicroseconds(waveLength - halfWaveLength); //for odd wavelengths
		*outIRpin &= ~IRpin;
		delayMicroseconds(halfWaveLength);

		microsecs -= waveLength;
	}

	interrupts();
}

// 
void sendCode(const uint16_t *code)
{
	trace("sendCode\r\n");
	
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
			return;
	}

	trace("sent\r\n");
}
