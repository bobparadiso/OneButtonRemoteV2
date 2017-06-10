#include <SD.h>
#include <SPI.h>
#include "speech.h"
#include "display.h"
#include "IR.h"
#include "Terminal.h"

#include "Trace.h"

#define CMD_RECORD "record"
#define CMD_PLAY "play"
#define CMD_DUMP "dump"

//
void terminalCmdHandler(char *cmd)
{
	if (strcmp(cmd, CMD_RECORD) == 0)
	{
		char *name = strtok(NULL, " \r\n");
		tracef("record:%s\r\n", name);
		
		if (strlen(name) > 8)
		{
			trace("ERROR; max filename length is 8\r\n");
			return;
		}
		
		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		CaptureIR(IRbuf);
		trace("captured:\r\n");
		logIR(IRbuf);
		storeIR(name, IRbuf);
	}
	else if (strcmp(cmd, CMD_PLAY) == 0)
	{
		char *name = strtok(NULL, " \r\n");
		tracef("play:%s\r\n", name);

		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		loadIR(name, IRbuf);
		logIR(IRbuf);
		if (sendCode(IRbuf) != 0)
			trace("ERROR; transmitter not responding\r\n");
	}
	else if (strcmp(cmd, CMD_DUMP) == 0)
	{
		char *name = strtok(NULL, " \r\n");
		tracef("dump:%s\r\n", name);

		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		loadIR(name, IRbuf);
		logIR(IRbuf);
	}
	else if (cmd)
	{
		trace("ERROR\r\n");
	}
}
