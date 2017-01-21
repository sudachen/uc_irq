
#include <~sudachen/uc_irq/import.h>

#ifdef __nRF5x_UC__
#include <app_util_platform.h>
#endif

const UcIrqHandler uc_irq$Nil = { NULL, NULL };

void ucHandle_IRQ(UcIrqHandler *irqList)
{
    UcIrqHandler *irqPtr = irqList;
    while (irqPtr != UC_IRQ_LIST_NIL)
    {
        UcIrqHandler *p = irqPtr;
        irqPtr = irqPtr->next;
        if ( p->handler != NULL ) p->handler(p);
    }
}

UcIrqHandler *UC_vIRQ_1ms = UC_IRQ_LIST_NIL;

#ifdef __nRF5x_UC__

bool uc_irq$rtcIsStarted = false;

void RTC1_IRQHandler(void)
{
    ucHandle_IRQ(UC_TIMED_IRQ);
    NRF_RTC1->TASKS_CLEAR = 1;
    __SEV();
}

#endif

void ucEnable_Irq(UcIrqNo irqNo,UcIrqPriority prio)
{
    int realPrio = 15;
    __Assert(prio >= UC_HIGH_PRIORITY_IRQ && prio <= UC_APP_PRIORITY_IRQ );

#ifdef __nRF5x_UC__
    switch(prio) {
        case UC_HIGH_PRIORITY_IRQ: realPrio = APP_IRQ_PRIORITY_HIGH; break;
        case UC_TIMER_PRIORITY_IRQ:realPrio = APP_IRQ_PRIORITY_MID; break;
        case UC_LOW_PRIORITY_IRQ:  realPrio = APP_IRQ_PRIORITY_LOW; break;
        case UC_APP_PRIORITY_IRQ:  realPrio = APP_IRQ_PRIORITY_THREAD; break;
        default: realPrio = APP_IRQ_PRIORITY_THREAD;
    }
#elif defined __stm32Fx_UC__
    switch(prio) {
        case UC_HIGH_PRIORITY_IRQ: realPrio  = 0; break;
        case UC_TIMER_PRIORITY_IRQ:realPrio  = 2; break;
        case UC_LOW_PRIORITY_IRQ:  realPrio  = 3; break;
        case UC_APP_PRIORITY_IRQ:  realPrio  = 4; break;
        default: realPrio = 4;
    }
#endif

    NVIC_SetPriority(irqNo,realPrio);
    NVIC_ClearPendingIRQ(irqNo);
    NVIC_EnableIRQ(irqNo);
}

void ucDisable_Irq(UcIrqNo irqNo)
{
    NVIC_DisableIRQ(irqNo);
}

#ifndef SOFTDEVICE_PRESENT
void uc_irq$requireLfclk()
{
    if ( NRF_CLOCK->EVENTS_LFCLKSTARTED )
    {
        // ???
        // setup low frequency clock source here
        NRF_CLOCK->LFCLKSRC             = (CLOCK_LFCLKSRC_SRC_Synth << CLOCK_LFCLKSRC_SRC_Pos);
        NRF_CLOCK->EVENTS_LFCLKSTARTED  = 0;
        NRF_CLOCK->TASKS_LFCLKSTART     = 1;
        while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) (void)0;
    }
}
#endif

#ifdef __nRF5x_UC__
void uc_irq$startRTC1()
{
#ifndef SOFTDEVICE_PRESENT
    uc_irq$requireLfclk();
#endif
    NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->TASKS_START = 1;
    ucEnable_Irq(RTC1_IRQn,UC_HIGH_PRIORITY_IRQ);
}

void uc_irq$stopRTC1()
{
    ucDisable_Irq(RTC1_IRQn);
}
#endif

#ifdef __stm32Fx_UC__
// it uses raw SysTick_Handler in HAL
void ucSysTick1ms()
    ucHandle_IRQ(UC_TIMED_IRQ)
}
#endif

void ucRegister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList)
{
    if ( irq->next != NULL ) return;
    irq->next = *irqList;
    *irqList = irq;
}

void ucUnregister_IrqHandler(UcIrqHandler *irq, UcIrqHandler **irqList)
{
    UcIrqHandler **ptr = irqList;
    if ( irq->next == NULL ) return;

    while ( *ptr != UC_IRQ_LIST_NIL && *ptr != irq ) ptr = &(*ptr)->next;

    if ( *ptr != UC_IRQ_LIST_NIL )
    {
        *ptr = (*ptr)->next;
        irq->next = 0;
    }
}

void ucRegister_1msHandler(UcIrqHandler *irq)
{
    ucRegister_IrqHandler(irq, &UC_vIRQ_1ms);
#ifdef __nRF5x_UC__
    if ( UC_vIRQ_1ms != UC_IRQ_LIST_NIL && !uc_irq$rtcIsStarted )
        uc_irq$startRTC1();
#endif
}

void ucUnregister_1msHandler(UcIrqHandler *irq)
{
    ucUnregister_IrqHandler(irq, &UC_vIRQ_1ms);
#ifdef __nRF5x_UC__
    if ( UC_vIRQ_1ms == UC_IRQ_LIST_NIL && uc_irq$rtcIsStarted )
        uc_irq$stopRTC1();
#endif
}
