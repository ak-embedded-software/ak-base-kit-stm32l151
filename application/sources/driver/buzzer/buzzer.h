#ifndef __BUZZER_H__
#define __BUZZER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "io_cfg.h"

#include "buzzer_music.h"

// Function prototypes
void BUZZER_Init(void);
void BUZZER_Enable(uint16_t freq, uint32_t duration);
void BUZZER_Disable(void);
void BUZZER_PlayTones(const Tone_TypeDef * melody);
void BUZZER_Sleep(bool sleep);

extern void buzzer_irq( void );

#ifdef __cplusplus
}
#endif

#endif // __BUZZER_H__
