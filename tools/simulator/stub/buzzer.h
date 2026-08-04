#ifndef __BUZZER_H__
#include <stdbool.h>
#define __BUZZER_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
typedef struct { uint16_t frequency; uint8_t duration; } Tone_TypeDef;
static const Tone_TypeDef tones_cc[]      = {{2000,2},{0,0}};
static const Tone_TypeDef tones_BUM[]     = {{3000,3},{0,0}};
static const Tone_TypeDef tones_startup[] = {{2000,3},{0,0}};
static const Tone_TypeDef tones_3beep[]   = {{4000,3},{0,0}};
static const Tone_TypeDef tones_SMB[]     = {{2637,18},{0,0}};
static const Tone_TypeDef tones_Lets_go[] = {{262,100},{0,0}};
static const Tone_TypeDef tones_USB_con[] = {{400,4},{0,0}};
static const Tone_TypeDef tones_USB_dis[] = {{1600,4},{0,0}};
static const Tone_TypeDef tones_merryChristmas[] = {{2637,9},{0,0}};
void BUZZER_Init(void);
void BUZZER_PlayTones(const Tone_TypeDef* m);
void BUZZER_Disable(void);
void BUZZER_Sleep(bool s);
#ifdef __cplusplus
}
#endif
#endif
