#ifndef __BUZZER_H__
#define __BUZZER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "io_cfg.h"
#include "buzzer_music.h"

#define BUZZER_SILENT_ON      (0U)
#define BUZZER_SILENT_OFF     (1U)

// Function prototypes
void BUZZER_Init(void);
void BUZZER_Enable(uint16_t freq, uint32_t duration);
void BUZZER_Disable(void);
void BUZZER_PlaySound(buzzer_sound_t sound);
void BUZZER_Silent(bool isSilent);

extern void buzzer_irq( void );

#ifdef __cplusplus
}
#endif

#endif // __BUZZER_H__
