#ifndef __IR_RX_
#define __IR_RX_

void IR_RX_init();
void IR_RX_start();
void IR_RX_stop();
void CaptureIR(uint16_t *buf);

#endif
