#include <SD.h>
#include <SPI.h>
#include "speech.h"
#include "display.h"
#include "Menu.h"
#include "Terminal.h"
#include "IR.h"
#include "Trace.h"

#define BUTTON_PIN 6

//
void setup()
{
	Serial.begin(115200);
	
#if defined(SEND_CODE_IR)
	setupIR_TX();
#elif defined(SEND_CODE_IR_RF)
	setupIR_TX_RF();
#endif
		
	IR_RX_init();
	IR_codes_init();

	setupDisplay();
	setupSpeech();

	loadMenus();
	currentMenu = 0;
	
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), BtnPressInterrupt, FALLING);
	
	setDisplayBacklight(255);
	say("hello");
	delay(500);
	setDisplayBacklight(0);
	display(".");
	
	int8_t ID;

	Serial.print(F("Waiting for button press.\r\n"));

	while (1)
	{
		//button not pressed
		if (digitalReadFast(BUTTON_PIN) == HIGH)
		{
			processSerial();
			continue;
		}
		
		setDisplayBacklight(255);
		waitForButtonRelease();			

		uint8_t prevMenu;
		do
		{
			prevMenu = currentMenu;
			runMenu(currentMenu);
		}
		while (prevMenu != currentMenu);
		
		setDisplayBacklight(0);
		display(".");
	}
}

//
void loop() {}
