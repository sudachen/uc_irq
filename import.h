
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

void register_irqHandler(struct UcIrqHandler *irq, UcIrqHandler **irqList);
void unregister_irqHandler(struct UcIrqHandler *irq, UcIrqHandler **irqList);
void handle_irq(struct UcIrqHandler *irqList);

extern UcIrqHandler *UC_vIRQ_1ms; // virtual IRQ signalled every 1ms
#define UC_TIMED_IRQ UC_vIRQ_1ms

void register_1msHandler(struct UcIrqHandler *irq);
void unregister_1msHandler(struct UcIrqHandler *irq);

#if __CORTEX_M >= 3
UcIrqPriority set_irqLevel(UcIrqPriority irqLevel);
#endif

void enable_appIrq(bool nested);
bool disable_appIrq(void); // return true if call is nested

#define __Critical \
    switch (0) for ( bool uc_irq$nested; 0; enable_appIrq(uc_irq$nested) ) \
        if(1) { case 0: uc_irq$nested = disable_appIrq(); goto C_LOCAL_ID(doIt); } \
        else C_LOCAL_ID(doIt):
