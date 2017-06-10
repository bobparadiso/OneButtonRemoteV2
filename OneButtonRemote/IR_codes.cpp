#include <SD.h>
#include <SPI.h>

#include "IR_codes.h"
#include "Trace.h"

#define SD_CS_PIN BUILTIN_SDCARD

//
void IR_codes_init()
{
	if (!SD.begin(SD_CS_PIN))
	{
		trace("SD Card initialization failed!\r\n");
		return;
	}
}

//
void storeIR(const char *name, uint16_t *data)
{
	//calc data len
	uint16_t *ptr = data;
	uint16_t carrier = *ptr++;
	while (1)
	{
		uint16_t ton = *ptr++;
		uint16_t toff = *ptr++;
		if (!toff)
			break;
	}
	uint16_t dataLen = ptr - data;

	//remove previous file if it exists
	if (SD.exists(name))
	{
		tracef("storeIR; IR_file:%s already exists, replacing\r\n", name);
		SD.remove(name);
	}
	
	File file = SD.open(name, FILE_WRITE);
	file.write((uint8_t *)data, dataLen * 2);
	file.close();
}

//
void loadIR(const char *name, uint16_t *data)
{
	//check if file exists
	if (!SD.exists(name))
	{
		tracef("loadIR; IR_file:%s does not exist!\r\n", name);
		return;
	}
	
	File file = SD.open(name, FILE_READ);
	//TODO: check file size against max buffer size
	file.read(data, file.available());
	file.close();
}

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

		if (!toff)
			break;
	}

	uint16_t len = (ptr - data - 1) / 2;
	tracef("IR_RX_RAW data END; carrier:%u len:%u\r\n", carrier, len);
}
