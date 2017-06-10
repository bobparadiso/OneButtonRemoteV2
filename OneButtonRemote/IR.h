#ifndef __IR_H_
#define __IR_H_

#define SEND_CODE_IR
//#define SEND_CODE_IR_RF

#include "IR_codes.h"

#if defined(SEND_CODE_IR)
	#include "IR_tx.h"
#elif defined(SEND_CODE_IR_RF)
	#include "IR_tx_RF.h"
#endif

#include "IR_rx.h"

#if defined(SEND_CODE_IR)
	#define sendCode sendCode_IR
#elif defined(SEND_CODE_IR_RF)
	#define sendCode sendCode_IR_RF
#endif

#endif
