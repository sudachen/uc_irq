
#include <~sudachen/uc_irq/import.h>

#ifdef __nRF5x_UC__
#include <nrf_delay.h>
#endif

const UcIrqHandler uc_irq$Nil = { NULL, NULL };

void handle_irq(UcIrqHandler *irqList)
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
    NRF_RTC1->EVENTS_TICK = 0;
    NRF_RTC1->EVENTS_OVRFLW = 0;
    handle_irq(UC_TIMED_IRQ);
    //__SEV();
}

#endif

__Forceinline int uc_irq$prio(UcIrqPriority prio)
{
#ifdef __nRF5x_UC__
    switch(prio) {
        case UC_HIGH_PRIORITY_IRQ: return APP_IRQ_PRIORITY_HIGH;
        case UC_TIMER_PRIORITY_IRQ:return  APP_IRQ_PRIORITY_MID;
        case UC_LOW_PRIORITY_IRQ:  return  APP_IRQ_PRIORITY_LOW;
        case UC_APP_PRIORITY_IRQ:  /* falldown */
        default:                   return  APP_IRQ_PRIORITY_THREAD;
    }
#else
    switch(prio) {
        case UC_HIGH_PRIORITY_IRQ: return 0;
        case UC_TIMER_PRIORITY_IRQ:return 2;
        case UC_LOW_PRIORITY_IRQ:  return 3;
        case UC_APP_PRIORITY_IRQ:  /* falldown */
        default:                   return 4;
    }
#endif
}

void enable_irq(UcIrqNo irqNo,UcIrqPriority prio)
{
    int realPrio = uc_irq$prio(prio);
    __Assert(prio >= UC_HIGH_PRIORITY_IRQ && prio <= UC_APP_PRIORITY_IRQ);

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

void disable_irq(UcIrqNo irqNo)
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
    if ( uc_irq$rtcIsStarted ) return;

    NRF_RTC1->PRESCALER = APP_TIMER_PRESCALER;
    NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->TASKS_START = 1;
    nrf_delay_us(47);

    enable_irq(RTC1_IRQn,UC_HIGH_PRIORITY_IRQ);
    uc_irq$rtcIsStarted = true;
}

void uc_irq$stopRTC1()
{
    NRF_RTC1->INTENCLR = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENCLR = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_STOP = 1;
    nrf_delay_us(47);
    NRF_RTC1->TASKS_CLEAR = 1;
    disable_irq(RTC1_IRQn);
    uc_irq$rtcIsStarted = false;
}

#endif

#ifdef __stm32Fx_UC__
// it uses raw SysTick_Handler in HAL
void on_sysTick1ms()
{
    handle_irq(UC_TIMED_IRQ);
}
#endif

void register_irqHandler(UcIrqHandler *irq, UcIrqHandler **irqList)
{
    if ( irq->next != NULL ) return;
    irq->next = *irqList;
    *irqList = irq;
}

void unregister_irqHandler(UcIrqHandler *irq, UcIrqHandler **irqList)
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

void register_1msHandler(UcIrqHandler *irq)
{
    register_irqHandler(irq, &UC_vIRQ_1ms);
#ifdef __nRF5x_UC__
    if ( UC_vIRQ_1ms != UC_IRQ_LIST_NIL && !uc_irq$rtcIsStarted )
        uc_irq$startRTC1();
#endif
}

void unregister_1msHandler(UcIrqHandler *irq)
{
    unregister_irqHandler(irq, &UC_vIRQ_1ms);
#ifdef __nRF5x_UC__
    if ( UC_vIRQ_1ms == UC_IRQ_LIST_NIL && uc_irq$rtcIsStarted )
        uc_irq$stopRTC1();
#endif
}

#if __CORTEX_M >= 3 // CMSIS
bool is_irqLevelLower(UcIrqPriority irqLevel)
{
    uint32_t oldPrio;
    uint32_t realPrio = uc_irq$prio(irqLevel) << 4;
    oldPrio = __get_BASEPRI();
    return oldPrio < realPrio;
}

UcIrqPriority set_irqLevel(UcIrqPriority irqLevel)
{
    uint32_t oldPrio;
    uint32_t realPrio = uc_irq$prio(irqLevel) << 4;
    oldPrio = __get_BASEPRI();
    __set_BASEPRI(realPrio);
    return oldPrio;
}
#endif

bool disable_appIrq()
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    uint8_t nestedCriticalReqion = 0;
    __Assert_Success sd_nvic_critical_region_enter(&nestedCriticalReqion);
    return nestedCriticalReqion;
#elif __CORTEX_M >= 3 // CMSIS
    if ( is_irqLevelLower(UC_HIGH_PRIORITY_IRQ))
    {
        set_irqLevel(UC_HIGH_PRIORITY_IRQ);
        return false;
    }
    return true;
#else
    uint32_t primask = __get_PRIMASK();
    if ( primask )
    {
        return true;
    }
    else
    {
        __disable_irq();
        return false;
    }
#endif
}

void enable_appIrq(bool nested)
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Success sd_nvic_critical_region_exit(nested);
#elif __CORTEX_M >= 3 // CMSIS
    if ( !nested ) set_irqLevel(UC_APP_PRIORITY_IRQ);
#else
    if ( !netsed ) __enable_irq();
#endif
}
