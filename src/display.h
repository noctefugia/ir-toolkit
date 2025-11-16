#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "utils.h"
	
typedef enum {
	DS_0,
	DS_1,
	DS_2,
	DS_3,
	DS_4,
	DS_5,
	DS_6,
	DS_7,
	DS_8,
	DS_9,
	DS_A,
	DS_B,
	DS_C,
	DS_D,
	DS_E,
	DS_F,
	DS_U,
	DS_L,
	DS_P,
	DS_R,
	DS_N,
	DS_O,
	DS_I,
	DS_J,
	DS_Y,
	DS_T,
	DS_H,
	DS_SEG_A,
	DS_SEG_B,
	DS_SEG_C,
	DS_SEG_D,
	DS_SEG_E,
	DS_SEG_F,
	DS_SEG_G,
	DS_SEG_AB,
	DS_SEG_ABCD,
	DS_SEG_ABCDE,
	DS_LOCK,
	DS_NONE,
	DS_NULL
} DispSymbol_TypeDef;

struct display_struct {
	struct io_struct io_seg[8], io_flash;
	uint16_t time_on, time_off;
	DispSymbol_TypeDef cur_symbol;
	uint8_t str_char_time, str_space_time, str_end_time, str_cur_pos, str_cur_time;
	uint8_t *str_pointer;
	bool state, enabled, str_mode, str_loop, flash_enabled;
};

extern const uint8_t DISP_STR_USB[];
extern const uint8_t DISP_STR_HELLO[];
extern const uint8_t DISP_STR_LOADING[];
extern const uint8_t DISP_STR_CHARGING[];
extern const uint8_t DISP_STR_UNLOCKING[];
extern const uint8_t DISP_STR_UNLOCKED[];
extern const uint8_t DISP_STR_STARTUP[];
extern const uint8_t DISP_STR_FLOOD[];
extern const uint8_t DISP_STR_BRUTE[];

void Display_Init(bool enable, uint16_t pwm_period, uint8_t pwm_brightness, DispSymbol_TypeDef symbol);
void DisplayEnable(bool state);
void Display_SetSymbol(DispSymbol_TypeDef type);
uint16_t Display_UpdatePWM(void);
void Display_SetBrightnessPWM(uint16_t pwm_period, uint8_t percent);
static void Display_On(void);
static void Display_Off(void);
void Display_StringSet(uint8_t *str, uint16_t t_char, uint16_t t_space, uint16_t t_end, uint16_t upd_period, bool loop);
void Display_StringUpdate(void);
bool Display_StringIsActive(void);
void Display_WriteDot(IO_MODE_TypeDef mode);
DispSymbol_TypeDef Display_GetCurSymbol(void);
void Display_EnableFlash(bool state);
bool Display_IsEnabled(void);
bool Display_FlashIsEnabled(void);

#endif /* __DISPLAY_H */