#include "speech.h"
#include "display.h"
#include "IR_codes.h"
#include "IR_tx_RF.h"
#include "IR_rx.h"
#include "Trace.h"

#define BUTTON_PIN 5

#define DIGIT_DELAY 250

#define REPEAT_DELAY 250 //ms
#define DEBOUNCE_ITERATIONS 10000 //iterations
#define LED_DURATION 500 //ms

//#define MENU_ITEM_DELAY 500
//#define DIGIT_MENU_ITEM_DELAY 250
#define MENU_ITEM_DELAY 10
#define DIGIT_MENU_ITEM_DELAY 10

//
enum MAIN_MENU {TV, MAIN_MENU_SIZE};
char *getMainMenuStr(uint8_t ID)
{
	switch (ID)
	{
	case TV: return "t v";
	}
	
	return "error";
}

//
enum TV_MENU {TV_POWER, TV_VOLUME, TV_CHANNEL, TV_INPUT, ROOM, TV_MENU_SIZE};
char *getTVMenuStr(uint8_t ID)
{
	switch (ID)
	{
	case TV_POWER: return "power";	
	case TV_VOLUME: return "volume";	
	case TV_CHANNEL: return "channel";
	case TV_INPUT: return "input";
	case ROOM: return "room";
	}
	
	return "error";
}

//
enum VOL_MENU {VOL_UP, VOL_DOWN, VOL_MENU_SIZE};
char *getVolMenuStr(uint8_t ID)
{
	switch (ID)
	{
	case VOL_UP: return "up";
	case VOL_DOWN: return "down";
	}
	
	return "error";
}

//
enum ROOM_MENU {ROOM_LIVING_ROOM, ROOM_BEDROOM, ROOM_MENU_SIZE};
char *getRoomMenuStr(uint8_t ID)
{
	switch (ID)
	{
	case ROOM_LIVING_ROOM: return "living room";
	case ROOM_BEDROOM: return "bedroom";
	}
	
	return "error";
}

//
char *getNumStr(uint8_t num)
{
	switch (num)
	{
	case 0: return "0";
	case 1: return "1";
	case 2: return "2";
	case 3: return "3";
	case 4: return "4";
	case 5: return "5";
	case 6: return "6";
	case 7: return "7";
	case 8: return "8";
	case 9: return "9";
	}
	
	return "error";
}

//
volatile uint8_t buttonPressed = 0;
void BtnPressInterrupt()
{
	buttonPressed = 1;
}

#define CMD_RECORD "record"
#define CMD_PLAY "play"
#define CMD_DUMP "dump"

//
void processTerminalCmd(char *cmdStr)
{
	char *cmd = strtok(cmdStr, " ");
	
	if (strcmp(cmd, CMD_RECORD) == 0)
	{
		char *name = strtok(NULL, "\r\n");
		tracef("record:%s\r\n", name);
		
		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		CaptureIR(IRbuf);
		trace("captured:\r\n");
		logIR(IRbuf);
		storeIR(name, IRbuf);
	}

	if (strcmp(cmd, CMD_PLAY) == 0)
	{
		char *name = strtok(NULL, "\r\n");
		tracef("play:%s\r\n", name);

		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		loadIR(name, IRbuf);
		logIR(IRbuf);
		sendCode(IRbuf);
	}

	if (strcmp(cmd, CMD_DUMP) == 0)
	{
		char *name = strtok(NULL, "\r\n");
		tracef("dump:%s\r\n", name);

		uint16_t IRbuf[MAX_IR_CODE_TIMINGS * 2 + 1];
		memset(IRbuf, 0, sizeof(IRbuf));
		loadIR(name, IRbuf);
		logIR(IRbuf);
	}
}

// 
void setup()
{
	Serial.begin(115200);
	
	setupIR_TX_RF();
		
	IR_RX_init();
	IR_codes_init();

	setupDisplay();
	setupSpeech();

	pinMode(BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), BtnPressInterrupt, FALLING);
	
	setDisplayBacklight(255);
	say("hello");
	delay(500);
	setDisplayBacklight(0);
	display(".");
	
	int8_t ID;

	Serial.print(F("Waiting for button press.\r\n"));

	char cmdBuf[128];
	char cmdBufIdx = 0;
	
	while (1)
	{
		//button not pressed
		if (digitalReadFast(BUTTON_PIN) == HIGH)
		{
			if (Serial.available())
			{
				char c = Serial.read();
				Serial.write(c); //local echo
				if (c == '\r' || c == '\n')
				{
					cmdBuf[cmdBufIdx++] = 0; //null-terminate
					processTerminalCmd(cmdBuf);
					cmdBufIdx = 0;
					continue;
				}
				cmdBuf[cmdBufIdx++] = c;
			}
			continue;
		}
		
		waitForButtonRelease();
			
		//ID = runButtonMenu(getMainMenuStr, MAIN_MENU_SIZE, MENU_ITEM_DELAY);
		ID = TV; //only TV for now
		
		switch (ID)
		{
		case TV:
			setDisplayBacklight(255);
			ID = runButtonMenu(getTVMenuStr, TV_MENU_SIZE, MENU_ITEM_DELAY);
			executeTVCommand(ID);
			setDisplayBacklight(0);
			display(".");
			break;
		}
	}
}

//
#define CHANNEL_DIGITS 2 //bptxxx

//
void setChannel()
{
	int ID;
	uint8_t digits[CHANNEL_DIGITS];

	for (int i = 0; i < CHANNEL_DIGITS; i++)
	{
		ID = -1;
		say("choose digit");
		while (ID == -1)
			ID = runButtonMenu(getNumStr, 10, DIGIT_MENU_ITEM_DELAY);
		digits[i] = ID;
	}
	
	trace(F("channel: "));
	for (int i = 0; i < CHANNEL_DIGITS; i++)
		tracef(F("%d"), digits[i]);
	trace(F("\n"));

	say("setting channel");
	for (int i = 0; i < CHANNEL_DIGITS; i++)
		say(getNumStr(digits[i]));
	
	for (int i = 0; i < CHANNEL_DIGITS; i++)
	{
		switch (digits[i])
		{
		case 0: sendCode("tv0"); break;
		case 1: sendCode("tv1"); break;
		case 2: sendCode("tv2"); break;
		case 3: sendCode("tv3"); break;
		case 4: sendCode("tv4"); break;
		case 5: sendCode("tv5"); break;
		case 6: sendCode("tv6"); break;
		case 7: sendCode("tv7"); break;
		case 8: sendCode("tv8"); break;
		case 9: sendCode("tv9"); break;
		}
		
		delay(DIGIT_DELAY);
	}	
}

//
void executeVolCommand(uint8_t ID)
{
	uint8_t i;	
	switch (ID)
	{
	case VOL_UP:
		for (i = 0; (i < 6 || digitalRead(BUTTON_PIN) == LOW); i++)
		{ 
			sendCode("tvvup");
			delay(REPEAT_DELAY);
		} 
		break;

	case VOL_DOWN:
		for (i = 0; (i < 6 || digitalRead(BUTTON_PIN) == LOW); i++)
		{ 
			sendCode("tvvdwn");
			delay(REPEAT_DELAY);
		} 
		break;
	}
}

//
void executeRoomCommand(uint8_t ID)
{
	uint8_t i;	
	switch (ID)
	{
	case ROOM_LIVING_ROOM: setTxAddr(LIVING_ROOM_ADDRESS); break;
	case ROOM_BEDROOM: setTxAddr(BEDROOM_ADDRESS); break;
	}
}

//
void executeTVCommand(uint8_t ID)
{
	uint8_t i;	
	switch (ID)
	{
	case TV_POWER: sendCode("tvpow"); break;

	case TV_VOLUME:
		ID = runButtonMenu(getVolMenuStr, VOL_MENU_SIZE, MENU_ITEM_DELAY);
		executeVolCommand(ID);
		break;
		
	case TV_CHANNEL: setChannel(); break;

	case TV_INPUT:
		sendCode("tvinput");
		delay(REPEAT_DELAY);

		for (i = 0; (i < 1 || digitalRead(BUTTON_PIN) == LOW); i++)
		{ 
			sendCode("tvinput");
			delay(500);
		} 
		break;

	case ROOM:
		ID = runButtonMenu(getRoomMenuStr, ROOM_MENU_SIZE, MENU_ITEM_DELAY);
		executeRoomCommand(ID);
		break;
	} 
}

//
void waitForButtonRelease()
{
	Serial.print(F("Waiting for button release.\r\n"));
	uint32_t i = 0;
	while (1)
	{
		if (digitalRead(BUTTON_PIN) == HIGH)
			i++;
		else
			i = 0;
			
		if (i == DEBOUNCE_ITERATIONS)
			return;
	}
}

//say each menu item out loud, and check if button is pressed (selecting it)
int8_t runButtonMenu(char *(*strFunc)(uint8_t), uint8_t menuSize, uint16_t itemDelay)
{
	waitForButtonRelease();
	
	//iterate through commands
	int8_t ID = 0;
	buttonPressed = 0;
	while (1)
	{
		say(strFunc(ID));
		delay(itemDelay);

		Serial.print(F("checking button\r\n"));
		if (buttonPressed)
		{
			say(strFunc(ID));
			break;
		}

		ID++;
		if (ID >= menuSize)
		{
			ID = -1;
			break;
		}
	}

	return ID;
}

//
void loop() {}
