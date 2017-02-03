
/*
 *
 * Stm32Fx HAL sets 1ms interval for SysTick timer by default,
 *   please do not change this behaviour!
 *
 */


#pragma once
#include <uccm/board.h>

#ifdef __nRF5x_UC__
#include <app_util_platform.h>
#include <app_timer.h>
#define APP_TIMER_PRESCALER 32
#endif

#pragma uccm require(source) += [@inc]/~sudachen/uc_irq/uc_irq.c

typedef enum UcIrqPriority UcIrqPriority;

enum UcIrqPriority
{
    UC_HIGH_PRIORITY_IRQ = 1,
    UC_TIMER_PRIORITY_IRQ,
    UC_LOW_PRIORITY_IRQ,
    UC_APP_PRIORITY_IRQ,
};

#if defined __nRF5x_UC__ || defined __stm32Fx_MCU__
typedef IRQn_Type UcIrqNo;
#else
typedef int UcIrqNo;
#endif

typedef struct UcIrqHandler UcIrqHandler;
struct UcIrqHandler
{
    void (*handler)(UcIrqHandler *self);
    UcIrqHandler *next;
};

extern const UcIrqHandler uc_irq$Nil; // place it into ROM
#define UC_IRQ_LIST_NIL ((UcIrqHandler*)&uc_irq$Nil) // Yep, I know what I do

void ucRegister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList);
void ucUnregister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList);
void ucHandle_IRQ(UcIrqHandler *irqList);

extern UcIrqHandler *UC_vIRQ_1ms; // virtual IRQ signalled every 1ms
#define UC_TIMED_IRQ UC_vIRQ_1ms

void ucRegister_1msHandler(UcIrqHandler *irq);
void ucUnregister_1msHandler(UcIrqHandler *irq);

#if __CORTEX_M >= 3
UcIrqPriority ucSet_IrqLevel(UcIrqPriority irqLevel);
#endif

void ucEnable_AppIrq(bool nested);
bool ucDisable_AppIrq(void); // return true if call is nested

#define __Critical \
    switch (0) for ( bool uc_irq$nested; 0; ucEnable_AppIrq(uc_irq$nested) ) \
        if(1) { case 0: uc_irq$nested = ucDisable_AppIrq(); goto C_LOCAL_ID(doIt); } \
        else C_LOCAL_ID(doIt):
