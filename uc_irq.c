
#include <~sudachen/uc_irq/import.h>

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
    ucToggle_BoardLED(1);
    NRF_RTC1->EVENTS_TICK = 0;
    ucHandle_IRQ(UC_TIMED_IRQ);
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

void ucEnable_Irq(UcIrqNo irqNo,UcIrqPriority prio)
{
    uint32_t err;
    int realPrio = uc_irq$prio(prio);
    __Assert(prio >= UC_HIGH_PRIORITY_IRQ && prio <= UC_APP_PRIORITY_IRQ );

#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Nrf_Success sd_nvic_SetPriority(irqNo,realPrio);
    __Assert_Nrf_Success sd_nvic_ClearPendingIRQ(irqNo);
    __Assert_Nrf_Success sd_nvic_EnableIRQ(irqNo);
#else
    NVIC_SetPriority(irqNo,realPrio);
    NVIC_ClearPendingIRQ(irqNo);
    NVIC_EnableIRQ(irqNo);
#endif
}

void ucDisable_Irq(UcIrqNo irqNo)
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Nrf_Success sd_nvic_DisableIRQ(irqNo);
#else
    NVIC_DisableIRQ(irqNo);
#endif
}

#ifdef __nRF5x_UC__
void uc_irq$startRTC1()
{
    if ( uc_irq$rtcIsStarted ) return;

    NRF_RTC1->PRESCALER = 32;
    NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->TASKS_START = 1;

    ucEnable_Irq(RTC1_IRQn,UC_HIGH_PRIORITY_IRQ);
    uc_irq$rtcIsStarted = true;
}

void uc_irq$stopRTC1()
{
    NRF_RTC1->TASKS_STOP = 1;
    ucDisable_Irq(RTC1_IRQn);
    uc_irq$rtcIsStarted = false;
}
#endif

#ifdef __stm32Fx_UC__
// it uses raw SysTick_Handler in HAL
void ucSysTick1ms()
{
    ucHandle_IRQ(UC_TIMED_IRQ);
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

#if __CORTEX_M >= 3 // CMSIS
bool ucIs_IrqLevelLower(UcIrqPriority irqLevel)
{
    uint32_t oldPrio;
    uint32_t realPrio = uc_irq$prio(irqLevel) << 4;
    oldPrio = __get_BASEPRI();
    return oldPrio < realPrio;
}

UcIrqPriority ucSet_IrqLevel(UcIrqPriority irqLevel)
{
    uint32_t oldPrio;
    uint32_t realPrio = uc_irq$prio(irqLevel) << 4;
    oldPrio = __get_BASEPRI();
    __set_BASEPRI(realPrio);
    return oldPrio;
}
#endif

bool ucDisable_AppIrq()
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    uint8_t nestedCriticalReqion = 0;
    __Assert_Nrf_Success sd_nvic_critical_region_enter(&nestedCriticalReqion);
    return nestedCriticalReqion;
#elif __CORTEX_M >= 3 // CMSIS
    if ( ucIs_IrqLevelLower(UC_HIGH_PRIORITY_IRQ))
    {
        ucSet_IrqLevel(UC_HIGH_PRIORITY_IRQ);
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

void ucEnable_AppIrq(bool nested)
{
#if defined __nRF5x_UC__ && defined SOFTDEVICE_PRESENT
    __Assert_Nrf_Success sd_nvic_critical_region_exit(nested);
#elif __CORTEX_M >= 3 // CMSIS
    if ( !nested ) ucSet_IrqLevel(UC_APP_PRIORITY_IRQ);
#else
    if ( !netsed ) __enable_irq();
#endif
}
