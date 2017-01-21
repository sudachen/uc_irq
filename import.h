
#pragma once
#include <uccm/board.h>

#pragma uccm require(end) += [@inc]/~sudachen/uc_irq/uc_irq.c

typedef struct UcIrqHandler UcIrqHandler;
struct UcIrqHandler
{
    void (*handler)(UcIrqHandler *self);
    UcIrqHandler *next;
};

extern UcIrqHandler uc_irq$Nil;
#define UC_IRQ_LIST_NIL uc_irq$Nil

#if defined NRF51 || defined NRF52
extern UcIrqHandler *UC_IRQ_RTC1;
#define UC_TIMED_IRQ UC_IRQ_RTC1
#elif defined STM32FX
extern UcIrqHandler *UC_IRQ_SysTick;
#define UC_TIMED_IRQ UC_IRQ_SysTick
#endif

void ucRegister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList);
void ucUnregister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList);
void ucHandle_IRQ(UcIrqHandler *irqList);
