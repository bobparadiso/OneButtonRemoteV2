#include <SD.h>
#include <SPI.h>
#include "speech.h"
#include "display.h"
#include "Terminal.h"
#include "IR.h"
#include "Trace.h"

#define BUTTON_PIN 6

#define DEBOUNCE_ITERATIONS 10000 //iterations

//slow
//#define MENU_ITEM_DELAY 500
//#define DIGIT_MENU_ITEM_DELAY 250
//fast
#define MENU_ITEM_DELAY 10
#define DIGIT_MENU_ITEM_DELAY 10

#define MENU_CONFIG_NAME "menu.txt"
#define MAX_OPTIONS 10 //and menus

enum OPTION_TYPE {OPTION_TYPE_MENU, OPTION_TYPE_ONCE, OPTION_TYPE_HOLD, OPTION_TYPE_NUMBER, OPTION_TYPE_UNDEF};

//
struct MenuOption
{
	char name[16];
	OPTION_TYPE type;
	char IR_cmd[9];
	uint16_t param[2];
	uint8_t loaded;
};

//
struct Menu
{
	MenuOption option[MAX_OPTIONS];
};

char mainMenuName[16];
uint8_t currentMenu;
Menu menu[MAX_OPTIONS + 1];

//
volatile uint8_t buttonPressed = 0;
void BtnPressInterrupt()
{
	buttonPressed = 1;
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

//
void fileReadLine(File file, char *buf, uint8_t size)
{
	char *ptr = buf;
	*ptr = 0;
	
	while (1)
	{
		//file finished
		if (!file.available())
		{
			*ptr = 0;
			return;
		}
		
		char c = file.read();
		
		//line finished
		if (c == '\r' || c == '\n')
		{
			*ptr = 0;
			if (ptr == buf)
				continue; //blank line, try next line
			else
				return; //finished
		}
		
		*ptr++ = c;
		
		//no more room left in buf, truncate
		if (ptr - buf == size - 1)
		{
			*ptr = 0;
			return;
		}
	}
}

//load all the menus from the configuration file
void loadMenus()
{	
	//check if file exists
	if (!SD.exists(MENU_CONFIG_NAME))
	{
		tracef("loadMenus; config file:%s does not exist!\r\n", MENU_CONFIG_NAME);
		return;
	}

	//clear menus
	memset(&menu, 0, sizeof(menu));
	
	File file = SD.open(MENU_CONFIG_NAME, FILE_READ);
	
	char buf[64];
	int8_t curMenu = -1;
	int8_t curOption;
	
	Menu *mainMenu = &menu[0];
	
	while (1)
	{
		fileReadLine(file, buf, sizeof(buf));
		
		//no more lines to read
		if (!strlen(buf))
			break;
		
		//new menu
		if (buf[0] == '!')
		{
			curMenu++;
			char *name = strtok(buf + 1, ",");

			//name main menu
			if (curMenu == 0)
			{
				strcpy(mainMenuName, name);
			}
			//store this menu as an option of main menu
			else
			{
				MenuOption *o = &mainMenu->option[curMenu - 1];
				strcpy(o->name, name);
				o->type = OPTION_TYPE_MENU;
				o->loaded = 1;
#ifdef SEND_CODE_IR_RF
				o->param[0] = atoi(strtok(NULL, ",")); //tx addr
#endif
				curOption = -1;
			}
		}
		
		//new option
		if (buf[0] == '*')
		{
			if (curMenu < 1)
				continue; //no current menu
			
			curOption++;
			MenuOption *o = &menu[curMenu].option[curOption];
			o->loaded = 1;

			//name
			char *name = strtok(buf + 1, ",");
			strcpy(o->name, name);
			
			//type
			char *type = strtok(NULL, ",");

			//IR_code
			char *code = strtok(NULL, ",");
			strcpy(o->IR_cmd, code);

			if (strcmp(type, "once") == 0)
			{
				o->type = OPTION_TYPE_ONCE;
			}
			else if (strcmp(type, "hold") == 0)
			{
				o->type = OPTION_TYPE_HOLD;
				o->param[0] = atoi(strtok(NULL, ",")); //min repeats
				o->param[1] = atoi(strtok(NULL, ",")); //repeat delay
			}
			else if (strcmp(type, "number") == 0)
			{
				o->type = OPTION_TYPE_NUMBER;
				o->param[0] = atoi(strtok(NULL, ",")); //num digits
				o->param[1] = atoi(strtok(NULL, ",")); //digit delay
			}
			else
			{
				o->type = OPTION_TYPE_UNDEF;
			}
		}
	}
	
	file.close();
}

//say each digit out loud, and check if button is pressed (selecting it)
uint8_t chooseDigit()
{
	waitForButtonRelease();
	
	//iterate through digits
	uint8_t ID = 0;
	buttonPressed = 0;
	while (1)
	{
		char buf[2];
		sprintf(buf, "%d", ID);
		say(buf);
		delay(DIGIT_MENU_ITEM_DELAY);

		Serial.print(F("checking button\r\n"));
		
		//digit selected
		if (buttonPressed)
		{
			say(buf);
			break;
		}

		ID++;
		if (ID == 10)
			ID = 0; //loop
	}

	return ID;
}
//used to select multi-digit numbers such as channel, station, etc.
#define MAX_DIGITS 10
void setNumber(char *name, char *codePrefix, uint8_t numDigits, uint16_t digitDelay)
{
	uint8_t digits[MAX_DIGITS];

	for (int i = 0; i < numDigits; i++)
	{
		say("choose digit");
		digits[i] = chooseDigit();
	}
	
	char msg[64];
	sprintf(msg, "setting %s", name);
	say(msg);
	msg[0] = 0;
	for (int i = 0; i < numDigits; i++)
	{
		char buf[2];
		sprintf(buf, "%d", digits[i]);
		strcat(msg, buf);
		strcat(msg, " ");
	}
	say(msg);
	tracef("setting %s: %s\r\n", name, msg);
	
	for (int i = 0; i < numDigits; i++)
	{
		char buf[64];
		sprintf(buf, "%s%d", codePrefix, digits[i]);
		if (sendCode(buf) != 0)
		{
			say("error, transmitter not responding");
			return;
		}
		delay(digitDelay);
	}
}

//say each menu item out loud, and check if button is pressed (selecting it)
void runMenu(uint8_t menuID)
{
	MenuOption *menuSettings = NULL;
	//if not main menu, grab settings
	if (menuID != 0)
		menuSettings = &menu[0].option[menuID-1];
	
#ifdef SEND_CODE_IR_RF
	//set tx addr if applicable
	if (menuID != 0)
	{
		uint8_t addr = menuSettings->param[0];
		setTxAddr(addr);
	}
#endif

	//announce menu name
	char menuPhrase[32];
	if (menuID == 0)
		sprintf(menuPhrase, "%s menu", mainMenuName);
	else
		sprintf(menuPhrase, "%s menu", menuSettings->name);
	say(menuPhrase);
	
	waitForButtonRelease();
	
	//iterate through commands
	int8_t ID = 0;
	buttonPressed = 0;
	MenuOption *o;
	while (1)
	{
		o = &menu[menuID].option[ID];		
		if (!o->loaded)
		{
			ID = -1;
			break; //end of menu
		}
		
		say(o->name);
		delay(MENU_ITEM_DELAY);

		Serial.print(F("checking button\r\n"));
		if (buttonPressed)
			break; //option chosen

		ID++;
		if (ID == MAX_OPTIONS)
		{
			ID = -1;
			break; //end of menu
		}
	}

	//give 'exit-to-main-menu' option
	if (ID == -1)
	{
		if (menuID == 0)
			return;
		
		char mainMenuPhrase[32];
		sprintf(mainMenuPhrase, "%s menu", mainMenuName);
		
		say(mainMenuPhrase);
		delay(MENU_ITEM_DELAY);

		Serial.print(F("checking button\r\n"));
		
		//option chosen
		if (buttonPressed)
			currentMenu = 0;
		
		return;
	}
	
	//echo the selection
	if (o->type != OPTION_TYPE_MENU)
		say(o->name);
	
	//perform the selection
	switch (o->type)
	{
	case OPTION_TYPE_MENU:
		currentMenu = ID + 1;
		break;
		
	case OPTION_TYPE_ONCE:
		if (sendCode(o->IR_cmd) != 0)
			say("error, transmitter not responding");
		break;
		
	case OPTION_TYPE_HOLD:
	{
		uint8_t minIterations = o->param[0];
		for (int i = 0; (i < minIterations || digitalRead(BUTTON_PIN) == LOW); i++)
		{
			if (sendCode(o->IR_cmd) != 0)
			{
				say("error, transmitter not responding");
				break;
			}
			delay(o->param[1]);
		} 
		break;
	}
		
	case OPTION_TYPE_NUMBER:
		setNumber(o->name, o->IR_cmd, o->param[0], o->param[1]);
		break;
	}
}
