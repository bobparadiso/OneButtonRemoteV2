#ifndef __IR_CODES_H_
#define __IR_CODES_H_

#include <stdint.h>

#define MAX_IR_CODE_TIMINGS 256
void IR_codes_init();
void storeIR(const char *name, uint16_t *data);
void loadIR(const char *name, uint16_t *data);
void logIR(uint16_t *data);

#endif
