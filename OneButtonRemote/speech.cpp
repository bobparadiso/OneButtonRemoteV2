#include <Arduino.h>
#include "speech.h"
#include "display.h"
#include "Trace.h"

#define MAX_BOOT_DELAY 3000
#define MAX_SPEECH_DELAY 3000

#define speech Serial1

void cmd(char *str);
void say(char *str);

//
void setupSpeech()
{
	speech.begin(9600);
	delay(MAX_BOOT_DELAY);
}

//
void setVoice(int16_t i)
{
	char buf[10];
	sprintf(buf, "n%d", i);
	cmd(buf);
}

//
void setVolume(int16_t i)
{
	char buf[10];
	sprintf(buf, "v%d", i);
	cmd(buf);
}

//
void setWPM(int16_t i)
{
	char buf[10];
	sprintf(buf, "w%d", i);
	cmd(buf);
}

//
void cmd(char *str)
{
	//flush rx
	while (speech.available())
		speech.read();
		
	tracef(F("sending: %s\r\n"), str);
	
	speech.println(str);

	uint32_t start = millis();
	trace(F("waiting for speech response\r\n"));
	while (1)
	{
		uint32_t elapsed = millis() - start;
		if (speech.read() == ':')
		{
			tracef(F("got speech response in %d ms\r\n"), elapsed);
			break;
		}
	
		if (elapsed > MAX_SPEECH_DELAY)
		{
			trace(F("response took too long...\r\n"));			
			speech.print("x\n"); //stop playback			
			start = millis();
		}
	}
}

//
void say(char *str)
{
	display(str);
	static char cmdBuf[64];
	sprintf(cmdBuf, "s%s", str);
	cmd(cmdBuf);
}
