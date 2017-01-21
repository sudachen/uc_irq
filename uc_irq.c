
#include <~sudachen/uc_irq/import.h>

UcIrqHandler uc_irq$Nil = { NULL, NULL };

void ucHandle_IRQ(UcIrqHandler *irqList)
{
    UcIrqHandler *irqPtr = irqList;
    while (irqPtr != &uc_irq$Nil)
    {
        if ( irqPtr->handler != NULL ) irqPtr->handler(irqPtr);
        irqPtr = irqPtr->next;
    }
}

#if defined NRF51 || defined NRF52
UcIrqHandler *UC_IRQ_RTC1 = &uc_irq$Nil;
void RTC1_IRQHandler(void)
{
    ucHandle_IRQ(UC_IRQ_RTC1);
}
#elif defined STM32FX
// there is no RTC1 IRQ, only wakeup and alarm
#endif

#if defined NRF51 || defined NRF52
// there is no SysTick timer
UcIrqHandler *UC_IRQ_RTC1 = &uc_irq$Nil;
void RTC1_IRQHandler(void)
{
    ucHandle_IRQ(UC_IRQ_RTC1);
}
#elif defined STM32FX
#pragma uccm cflags+=-DUC_EXTEND_SYSTICK
// it uses raw SysTick_Handler in HAL
UcIrqHandler *UC_IRQ_SysTick = &uc_irq$Nil;
void ucSysTickHandler()
{
    ucHandle_IRQ(UC_IRQ_SysTick)
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
    while ( *ptr != &uc_irq$Nil && *ptr != irq ) ptr = &(*ptr)->next;
    if ( *ptr != &uc_irq$Nil )
    {
        *ptr = (*ptr)->next;
        irq->next = 0;
    }
}
