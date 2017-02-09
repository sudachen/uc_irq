
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

typedef enum IrqPriority IrqPriority;

enum IrqPriority
{
    HIGH_PRIORITY_IRQ = 1,
    TIMER_PRIORITY_IRQ,
    LOW_PRIORITY_IRQ,
    APP_PRIORITY_IRQ,
};

#if defined __nRF5x_UC__ || defined __stm32Fx_MCU__
typedef IRQn_Type IrqNo;
#else
typedef int IrqNo;
#endif

typedef struct IrqHandler IrqHandler;
struct IrqHandler
{
    void (*handler)(IrqHandler *self);
    IrqHandler *next;
};

extern const IrqHandler uc_irq$Nil; // place it into ROM
#define IRQ_LIST_NIL ((IrqHandler*)&uc_irq$Nil) // Yep, I know what I do

void register_irqHandler(struct IrqHandler *irq, IrqHandler **irqList);
void unregister_irqHandler(struct IrqHandler *irq, IrqHandler **irqList);
void handle_irq(struct IrqHandler *irqList);

extern IrqHandler *vIRQ_1ms; // virtual IRQ signalled every 1ms
#define TIMED_IRQ vIRQ_1ms

void register_1msHandler(struct IrqHandler *irq);
void unregister_1msHandler(struct IrqHandler *irq);

#if __CORTEX_M >= 3
void set_irqLevel(IrqPriority irqLevel);
#endif

void enable_appIrq(int nested);
int disable_appIrq(void); // return true if call is nested

#define __Critical \
    switch (0) for ( int uc_irq$nested; 0; enable_appIrq(uc_irq$nested) ) \
        if(1) { case 0: uc_irq$nested = disable_appIrq(); goto C_LOCAL_ID(doIt); } \
        else C_LOCAL_ID(doIt):
