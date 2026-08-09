#ifndef __BUZZER_MUSIC_H__
#define __BUZZER_MUSIC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// Single tone definition. Duration unit is 10 ms.
typedef struct {
	uint16_t frequency;
	uint8_t  duration;
} Tone_TypeDef;

typedef enum {
	BUZZER_SOUND_CLICK = 0,
	BUZZER_SOUND_BANG,
	BUZZER_SOUND_USB_CONNECTED,
	BUZZER_SOUND_USB_DISCONNECTED,
	BUZZER_SOUND_LETS_GO,
	BUZZER_SOUND_STARTUP,
	BUZZER_SOUND_3BEEP,
	BUZZER_SOUND_WELCOME,
	BUZZER_SOUND_GOODBYE,
	BUZZER_SOUND_HIGHSCORE,
	BUZZER_SOUND_LOWSCORE,
	BUZZER_SOUND_SUPER_MARIO,
	BUZZER_SOUND_MERRY_CHRISTMAS,
	BUZZER_SOUND_TONE_1,
	BUZZER_SOUND_TONE_2,
	BUZZER_SOUND_TONE_3,
	BUZZER_SOUND_TONE_4,
	BUZZER_SOUND_TONE_5,
	BUZZER_SOUND_TONE_6,
	BUZZER_SOUND_TONE_7,
	BUZZER_SOUND_MAX
} buzzer_sound_t;

extern const Tone_TypeDef tones_click[];
extern const Tone_TypeDef tones_bang[];
extern const Tone_TypeDef tones_usb_connected[];
extern const Tone_TypeDef tones_usb_disconnected[];
extern const Tone_TypeDef tones_lets_go[];
extern const Tone_TypeDef tones_startup[];
extern const Tone_TypeDef tones_3beep[];
extern const Tone_TypeDef tones_welcome[];
extern const Tone_TypeDef tones_goodbye[];
extern const Tone_TypeDef tones_highscore[];
extern const Tone_TypeDef tones_lowscore[];
extern const Tone_TypeDef tones_supper_mario_bros[];
extern const Tone_TypeDef tones_merry_christmas[];
extern const Tone_TypeDef sTone1[];
extern const Tone_TypeDef sTone2[];
extern const Tone_TypeDef sTone3[];
extern const Tone_TypeDef sTone4[];
extern const Tone_TypeDef sTone5[];
extern const Tone_TypeDef sTone6[];
extern const Tone_TypeDef sTone7[];

#ifdef __cplusplus
}
#endif

#endif // __BUZZER_MUSIC_H__
