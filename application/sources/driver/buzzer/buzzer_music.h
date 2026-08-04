/**
 ******************************************************************************
 * @brief: Tone tables for the buzzer.
 *
 * These live in buzzer_music.c, not in a header. Declaring them `static const`
 * in buzzer.h gave every translation unit that included it its own private
 * copy in flash - with 14 such units and multi-hundred-byte melodies that is
 * pure waste on a 116K part.
 ******************************************************************************
**/
#ifndef __BUZZER_MUSIC_H__
#define __BUZZER_MUSIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Single tone definition
typedef struct {
	uint16_t frequency;
	uint8_t  duration;
} Tone_TypeDef;

extern const Tone_TypeDef tones_cc[];
extern const Tone_TypeDef tones_BUM[];
extern const Tone_TypeDef tones_USB_con[];
extern const Tone_TypeDef tones_USB_dis[];
extern const Tone_TypeDef tones_Lets_go[];
extern const Tone_TypeDef tones_startup[];
extern const Tone_TypeDef tones_3beep[];
extern const Tone_TypeDef tones_SMB[];
extern const Tone_TypeDef tones_merryChristmas[];

#ifdef __cplusplus
}
#endif

#endif //__BUZZER_MUSIC_H__
