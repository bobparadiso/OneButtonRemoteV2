#include <Arduino.h>
#include "TimerOne.h" //from: https://github.com/PaulStoffregen/TimerOne

#include "Trace.h"
#include "IR_rx.h"
#include "IR_codes.h"

#define IR_RX_PIN 2

//make sure these are unused pins
//#define DEBUG1_PIN 33
//#define DEBUG2_PIN 34
//#define DEBUG3_PIN 35
//uint8_t debug3;

#define RAW_BURST_SIZE 3

uint16_t IR_rx_buf[MAX_IR_CODE_TIMINGS * RAW_BURST_SIZE];

//in us
#define TOFF_THRESHOLD 200 //a space greater than this is actually the Toff
#define MIN_BURST_LEN 200
#define IR_END_THRESHOLD 100000 //space longer than this is end of last IR code (including repeats)

#define US_IN_SEC 1000000

#define TIMEOUT_NONE 0
#define TIMEOUT_BURST 1
#define TIMEOUT_IR_CODE 2

uint8_t pending_timeout = TIMEOUT_NONE;

uint32_t prev_burst_end = 0;

uint16_t pulse_cnt = 0;
uint16_t pulse_time = 0;
uint32_t pulse_start = 0;
uint32_t burst_start = 0;
uint32_t burst_end = 0;

uint8_t burst_started = 0;

volatile uint16_t *IR_rx_cur;
volatile uint8_t IR_captured;

//
void CaptureIR(uint16_t *buf)
{
	IR_RX_start();
	Serial.println("ready for IR");
	
	while (!IR_captured);
	
	//copy captured IR into buf
	uint16_t *src_ptr = IR_rx_buf;
	uint16_t *dst_ptr = buf;
	uint32_t carrierTotal = 0; //interim variable to calc carrier
	uint16_t burstCount = 0;
	
	dst_ptr++; //skip over storing carrier for now
	
	while (1)
	{
		uint16_t carrier = *src_ptr++;
		uint16_t ton = *src_ptr++;
		uint16_t toff = *src_ptr++;
		
		if (carrier)
		{
			carrierTotal += carrier;
			burstCount++;
		}

		//tracef("%6u\t%4u\t%4u\r\n", carrier, ton, toff);

		*dst_ptr++ = ton;
		*dst_ptr++ = toff;
		
		if (!toff)
			break;
	}

	uint8_t len = (src_ptr - IR_rx_buf) / RAW_BURST_SIZE;
	uint16_t calculatedCarrier = carrierTotal / burstCount;
	//tracef("IR_RX_RAW data END; carrier:%lu len:%u\r\n", calculatedCarrier, len);
	
	*buf = calculatedCarrier; //finally stored calculated carrier
	
	IR_RX_stop();
}

//
void LogRawIR(uint16_t *data)
{
	trace("IR_RX_RAW data:\r\n");
	uint16_t *ptr = data;
	uint32_t carrierTotal = 0; //interim variable to calc carrier

	while (1)
	{
		uint16_t carrier = *ptr++;
		uint16_t ton = *ptr++;
		uint16_t toff = *ptr++;
		carrierTotal += carrier;

		tracef("%6u\t%4u\t%4u\r\n", carrier, ton, toff);

		if (!toff)
			break;
	}

	uint8_t len = (ptr - data) / RAW_BURST_SIZE;
	uint16_t calculatedCarrier = carrierTotal / len;
	tracef("IR_RX_RAW data END; carrier:%lu len:%u\r\n", calculatedCarrier, len);
}

//
void IR_RX_Finished()
{
	trace("finished\r\n");

	LogRawIR(IR_rx_buf);

	//reset
	IR_rx_cur = IR_rx_buf;
	IR_captured = 1;
}

//validates and stores burst info
//assumes that appropriate variables related to bursts and pulses are already set by calling interrupt
void processBurst()
{
	//trace("processBurst\r\n");
	
	//burst is valid(not too short and not too many missing pulses), store it
	uint16_t burst_len = burst_end - burst_start;
	if (pulse_cnt > 2 && burst_len >= MIN_BURST_LEN)
	{
		//we have a previous burst stored, update it's Toff
		if (IR_rx_cur > IR_rx_buf)
		{
			uint32_t off = burst_start - prev_burst_end;
			//during a code
			if (off <= 0xFFFF)
			{
				*(IR_rx_cur - 1) = off;
			}
			//handle extra long Toff in between code repeats
			else
			{
				*(IR_rx_cur - 1) = 0xFFFF;
				off -= 0xFFFF;

				//chunk the remaining Toff
				while (off)
				{
					*IR_rx_cur++ = 0;
					*IR_rx_cur++ = 0;
					uint16_t off_chunk = min(off, 0xFFFF);
					*IR_rx_cur++ = off_chunk;
					off -= off_chunk;
				}
			}
		}

		//previous burst was end of buffer, truncate
		if (IR_rx_cur > IR_rx_buf + MAX_IR_CODE_TIMINGS - RAW_BURST_SIZE)
		{
			*IR_rx_cur++ = 0;
			*IR_rx_cur++ = 0;
			*IR_rx_cur++ = 0;
			IR_RX_Finished();
		}
		//store carrier and Ton of current burst
		else
		{
			*(IR_rx_cur + 0) = US_IN_SEC / (burst_len / (pulse_cnt-1)); //calc carrier
			*(IR_rx_cur + 1) = burst_len; //Ton
			IR_rx_cur += RAW_BURST_SIZE;
		}

		prev_burst_end = burst_end;
	}

	//reset for next burst
	pulse_cnt = 0;
	burst_started = 0;
}

//timeout occurred, process burst
void IR_RX_TimeoutInterrupt()
{
	//digitalWrite(DEBUG2_PIN, HIGH); //debug
	
	//trace("time out\r\n");

	//end of burst
	if (pending_timeout == TIMEOUT_BURST)
	{
		burst_end = pulse_start; //burst end was the last pulse we saw
		processBurst();

		//set timeout for ending IR code
		pending_timeout = TIMEOUT_IR_CODE;
		Timer1.setPeriod(IR_END_THRESHOLD * 2); //this timer lib w/ this MCU you need to double the value
		Timer1.start();
		//digitalWriteFast(DEBUG3_PIN, (debug3 = !debug3)); //debug
	}
	//end of IR code
	else if (pending_timeout == TIMEOUT_IR_CODE)
	{
		//digitalWriteFast(DEBUG3_PIN, (debug3 = !debug3)); //debug

		//we have a previous burst stored, update it's Toff
		if (IR_rx_cur > IR_rx_buf)
			*(IR_rx_cur - 1) = 0;

		//end of IR code
		*IR_rx_cur++ = 0;
		*IR_rx_cur++ = 0;
		*IR_rx_cur++ = 0;
		IR_RX_Finished();

		pending_timeout = TIMEOUT_NONE;
		Timer1.stop();
	}
	
	//digitalWrite(DEBUG2_PIN, LOW); //debug
}

//pulse came in
void IR_RX_CaptureInterrupt()
{
	//digitalWrite(DEBUG1_PIN, HIGH); //debug
	
	//trace("capture\r\n");
	
	pulse_start = micros(); //set start for next pulse timing

	//set timeout for ending burst
	pending_timeout = TIMEOUT_BURST;
	Timer1.setPeriod(TOFF_THRESHOLD * 2); //this timer lib w/ this MCU you need to double the value
	Timer1.start();
	//digitalWriteFast(DEBUG3_PIN, (debug3 = !debug3)); //debug

	pulse_cnt++;

	//first pulse of next burst
	if (!burst_started)
	{
		burst_started = 1;
		burst_start = pulse_start;
	}

	//digitalWrite(DEBUG1_PIN, LOW); //debug
}

void IR_RX_init()
{
	trace("init\r\n");
	Timer1.initialize();
	Timer1.stop();
	
	//debug
	//pinMode(DEBUG1_PIN, OUTPUT);
	//pinMode(DEBUG2_PIN, OUTPUT);
	//pinMode(DEBUG3_PIN, OUTPUT);
}

//configure timers for IR_RX using raw sensor
void IR_RX_start()
{
	trace("start\r\n");

	IR_rx_cur = IR_rx_buf;
	IR_captured = 0;
	
    //capture interrupt
	attachInterrupt(digitalPinToInterrupt(IR_RX_PIN), IR_RX_CaptureInterrupt, FALLING);

    //timeout interrupt
	Timer1.attachInterrupt(IR_RX_TimeoutInterrupt);
}

//
void IR_RX_stop()
{
	trace("stop\r\n");
	
    //capture interrupt
	detachInterrupt(digitalPinToInterrupt(IR_RX_PIN));
	
    //timeout interrupt
	Timer1.detachInterrupt();
	Timer1.stop();
}
