#ifndef __SYS_IRQ_H__
#define __SYS_IRQ_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SYS_IRQ_EXCEPTION_NUMBER_IRQ0_NUMBER_RESPECTIVE		16	/* exception number 16 ~~ IRQ0 */

extern void sys_irq_nrf24l01();
extern void sys_irq_shell();
extern void sys_irq_uart2();
extern void sys_irq_timer_10ms();

#ifdef __cplusplus
}
#endif

#endif // __SYS_IRQ_H__
