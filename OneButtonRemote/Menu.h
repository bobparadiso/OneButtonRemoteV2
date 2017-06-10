#ifndef __MENU_H_
#define __MENU_H_

extern volatile uint8_t buttonPressed;
extern uint8_t currentMenu;

void BtnPressInterrupt();
void waitForButtonRelease();
void loadMenus();
void runMenu(uint8_t menuID);

#endif
