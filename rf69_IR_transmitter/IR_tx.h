#ifndef __IR_TX_H_
#define __IR_TX_H_

void setupIR_TX();
void pulseIR(uint16_t kHz, long microsecs);
void sendCode(const uint16_t *code);

#endif
