#ifndef __IR_TX_H_
#define __IR_TX_H_

void setupIR_TX();
int8_t sendCode_IR(const char *name);
int8_t sendCode_IR(const uint16_t *code);

#endif
