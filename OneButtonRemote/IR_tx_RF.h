#ifndef __IR_TX_RF_H_
#define __IR_TX_RF_H_

#define CONTROLLER_ADDRESS 1
#define LIVING_ROOM_ADDRESS 2
#define BEDROOM_ADDRESS 3

void setupIR_TX_RF();
void setTxAddr(uint8_t addr);
int8_t sendCode_IR_RF(const char *name);
int8_t sendCode_IR_RF(const uint16_t *code);

#endif
