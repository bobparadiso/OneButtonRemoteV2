#include <Arduino.h>
#include "display.h"
#include "Trace.h"

#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 2
#define DISPLAY_LEN (DISPLAY_WIDTH * DISPLAY_HEIGHT)

#include "LiquidCrystal_I2C.h"
#define BACKLIGHT_PIN (3)

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, BACKLIGHT_PIN, POSITIVE);
//LiquidCrystal_I2C lcd(0x26, 2, 1, 0, 4, 5, 6, 7, BACKLIGHT_PIN, POSITIVE);

//for display on a Raspberry Pi w/ large monitor if desired
#define DisplaySerial Serial2

//
void setupDisplay()
{
	// Switch on the backlight
	pinMode(BACKLIGHT_PIN, OUTPUT);
	digitalWrite(BACKLIGHT_PIN, HIGH);

	lcd.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT);
	
	lcd.home();
	lcd.cursor();
	lcd.setBacklight(0);
	
	DisplaySerial.begin(9600);
}

//
void setDisplayBacklight(uint8_t val)
{
	lcd.setBacklight(val);
}

//
void display(char *str)
{
	lcd.clear();
	lcd.home();
	lcd.print(str);
	DisplaySerial.println(str);
}
