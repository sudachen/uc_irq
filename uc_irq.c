
#include <~sudachen/uc_irq/import.h>

#ifdef __nRF5x_UC__
#include <nrf_delay.h>
#endif

const IrqHandler uc_irq$Nil = { NULL, NULL };

void handle_irq(IrqHandler *irqList)
{
    IrqHandler *irqPtr = irqList;
    while (irqPtr != IRQ_LIST_NIL)
    {
        IrqHandler *p = irqPtr;
        irqPtr = irqPtr->next;
        if ( p->handler != NULL ) p->handler(p);
    }
}

IrqHandler *IRQlist_1ms = IRQ_LIST_NIL;

#ifdef __nRF5x_UC__

bool uc_irq$RtcIsStarted = false;

void RTC1_IRQHandler(void)
{
    NRF_RTC1->EVENTS_TICK = 0;
    NRF_RTC1->EVENTS_OVRFLW = 0;
    handle_irq(TIMED_IRQ);
    //__SEV();
}

#endif

__Forceinline
int uc_irq$prio(IrqPriority prio)
{
#ifdef __nRF5x_UC__
    switch(prio) {
        case HIGH_PRIORITY_IRQ: return APP_IRQ_PRIORITY_HIGH;
        case TIMER_PRIORITY_IRQ:return  APP_IRQ_PRIORITY_MID;
        case LOW_PRIORITY_IRQ:  return  APP_IRQ_PRIORITY_LOW;
        case APP_PRIORITY_IRQ:  /* falldown */
        default:                return  APP_IRQ_PRIORITY_THREAD;
    }
#else
    switch(prio) {
        case HIGH_PRIORITY_IRQ: return 0;
        case TIMER_PRIORITY_IRQ:return 2;
        case LOW_PRIORITY_IRQ:  return 3;
        case APP_PRIORITY_IRQ:  /* falldown */
        default:                return 4;
    }
#endif
}

void enable_irq(IrqNo irqNo,IrqPriority prio)
{
    int realPrio = uc_irq$prio(prio);
    __Assert(prio >= HIGH_PRIORITY_IRQ && prio <= APP_PRIORITY_IRQ);

#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Success sd_nvic_SetPriority(irqNo,realPrio);
    __Assert_Success sd_nvic_ClearPendingIRQ(irqNo);
    __Assert_Success sd_nvic_EnableIRQ(irqNo);
#else
    NVIC_SetPriority(irqNo,realPrio);
    NVIC_ClearPendingIRQ(irqNo);
    NVIC_EnableIRQ(irqNo);
#endif
}

void disable_irq(IrqNo irqNo)
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Success sd_nvic_DisableIRQ(irqNo);
#else
    NVIC_DisableIRQ(irqNo);
#endif
}

#ifdef __nRF5x_UC__
void uc_irq$startRTC1()
{
    if ( uc_irq$RtcIsStarted ) return;

    NRF_RTC1->PRESCALER = APP_TIMER_PRESCALER;
    NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->TASKS_START = 1;
    nrf_delay_us(47);

    enable_irq(RTC1_IRQn,HIGH_PRIORITY_IRQ);
    uc_irq$RtcIsStarted = true;
}

void uc_irq$stopRTC1()
{
    NRF_RTC1->INTENCLR = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENCLR = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_STOP = 1;
    nrf_delay_us(47);

    NRF_RTC1->TASKS_CLEAR = 1;
    disable_irq(RTC1_IRQn);
    uc_irq$RtcIsStarted = false;
}
#endif

#ifdef __stm32Fx_UC__
// it uses raw SysTick_Handler in HAL
void on_sysTick1ms()
{
    handle_irq(TIMED_IRQ);
}
#endif

void register_irqHandler(IrqHandler *irq, IrqHandler **irqList)
{
    __Critical
    {
        if ( irq->next != NULL ) return;
        irq->next = *irqList;
        *irqList = irq;
    }
}

void unregister_irqHandler(IrqHandler *irq, IrqHandler **irqList)
{
    IrqHandler **ptr = irqList;
    if ( irq->next == NULL ) return;

    __Critical
    {
        while ( *ptr != IRQ_LIST_NIL && *ptr != irq ) ptr = &(*ptr)->next;

        if ( *ptr != IRQ_LIST_NIL )
        {
            *ptr = (*ptr)->next;
            irq->next = 0;
        }
    }

}

void register_1msHandler(IrqHandler *irq)
{
    register_irqHandler(irq, &IRQlist_1ms);
#ifdef __nRF5x_UC__
    if ( IRQlist_1ms != IRQ_LIST_NIL && !uc_irq$RtcIsStarted )
        uc_irq$startRTC1();
#endif
}

void unregister_1msHandler(IrqHandler *irq)
{
    unregister_irqHandler(irq, &IRQlist_1ms);
#ifdef __nRF5x_UC__
    if ( IRQlist_1ms == IRQ_LIST_NIL && uc_irq$RtcIsStarted )
        uc_irq$stopRTC1();
#endif
}
