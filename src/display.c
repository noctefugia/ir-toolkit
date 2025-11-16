#include "display.h"

struct display_struct disp = {0};

static const uint8_t disp_symbol_data[] = {
	0b00111111,	//0
	0b00000110,	//1
	0b01011011,	//2
	0b01001111, //3
	0b01100110, //4
	0b01101101, //5
	0b01111101, //6
	0b00000111, //7
	0b01111111, //8
	0b01101111, //9
	0b01110111, //A
	0b01111100, //b
	0b00111001, //C
	0b01011110, //d
	0b01111001, //E
	0b01110001, //F
	0b00111110, //U
	0b00111000, //L
	0b01110011, //P
	0b01010000, //r
	0b01010100, //n
	0b01011100, //o
	0b00110000, //I
	0b00001110, //J
	0b01101110, //Y
	0b01111000, //t
	0b01110110, //H
	0b00000001, //SEG-A
	0b00000010, //SEG-B
	0b00000100, //SEG-C
	0b00001000, //SEG-D
	0b00010000, //SEG-E
	0b00100000, //SEG-F
	0b01000000, //SEG-G
	0b00000011, //SEG-AB
	0b00001111, //SEG-ABCD
	0b00011111, //SEG-ABCDE
	0b01011101, //LOCK
	0x00, //none
	0xFF //NULL (used only for string processing)
};


const uint8_t DISP_STR_USB[] = {DS_U, DS_5, DS_B, DS_NULL};
const uint8_t DISP_STR_HELLO[] = {DS_H, DS_E, DS_L, DS_L, DS_O, DS_NULL};
const uint8_t DISP_STR_LOADING[] = {DS_SEG_A, DS_SEG_B, DS_SEG_C, DS_SEG_D, DS_SEG_E, DS_SEG_F, DS_NULL};
const uint8_t DISP_STR_CHARGING[] = {DS_SEG_D, DS_O, DS_0, DS_NULL};
const uint8_t DISP_STR_UNLOCKING[] = {DS_NONE, DS_SEG_A, DS_SEG_AB, DS_7, DS_SEG_ABCD, DS_SEG_ABCDE, DS_0, DS_NULL};
const uint8_t DISP_STR_UNLOCKED[] = {DS_0, DS_0, DS_0, DS_NULL};
const uint8_t DISP_STR_STARTUP[] = {DS_LOCK, DS_NULL};
const uint8_t DISP_STR_FLOOD[] = {DS_F, DS_L, DS_O, DS_O, DS_D, DS_NULL};
const uint8_t DISP_STR_BRUTE[] = {DS_B, DS_R, DS_U, DS_T, DS_E, DS_NULL};

void Display_Init(
	bool enable, 
	uint16_t pwm_period, uint8_t pwm_brightness, 
	DispSymbol_TypeDef symbol
)
{
	AssignIO(&disp.io_seg[0], PORT_DISP_SEG_A, PIN_DISP_SEG_A, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[1], PORT_DISP_SEG_B, PIN_DISP_SEG_B, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[2], PORT_DISP_SEG_C, PIN_DISP_SEG_C, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[3], PORT_DISP_SEG_D, PIN_DISP_SEG_D, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[4], PORT_DISP_SEG_E, PIN_DISP_SEG_E, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[5], PORT_DISP_SEG_F, PIN_DISP_SEG_F, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[6], PORT_DISP_SEG_G, PIN_DISP_SEG_G, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	AssignIO(&disp.io_seg[7], PORT_DISP_SEG_DP, PIN_DISP_SEG_DP, TRUE, GPIO_MODE_OUT_PP_HIGH_SLOW);
	
	AssignIO(&disp.io_flash, PORT_FLASHLIGHT, PIN_FLASHLIGHT, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	
	disp.cur_symbol = DS_NONE;
	disp.state = FALSE;
	Display_EnableFlash(FALSE);
	DisplayEnable(enable);
	Display_SetBrightnessPWM(pwm_period, pwm_brightness);
	Display_SetSymbol(symbol);
}


uint16_t Display_UpdatePWM(void)
{
	disp.state = !disp.state;
	
	if (disp.flash_enabled) {
		if (disp.state) {
			WriteIO(&disp.io_flash, IO_High);
			if (disp.enabled)
				Display_On();
			return disp.time_on;
		} else {
			WriteIO(&disp.io_flash, IO_Low);
			return disp.time_off;
		}
	} else if (!disp.enabled) {
		return (disp.time_on + disp.time_off);
	} else if (disp.state) {
		Display_On();
		return disp.time_on;
	} else {
		Display_Off();
		return disp.time_off;
	}
}


void Display_SetSymbol(DispSymbol_TypeDef type) {
	disp.cur_symbol = type;
	disp.str_mode = FALSE;
}


static void Display_On(void)
{
	uint8_t data, i;
	IO_MODE_TypeDef mode;
	
	data = disp_symbol_data[disp.cur_symbol];
	for (i = 0; i < 7; ++i) {
		mode = (data & 0x01) ? IO_Low : IO_High;
		WriteIO(&disp.io_seg[i], mode);
		data >>= 1;
	}
}


static void Display_Off(void)
{
	uint8_t i;
	
	for (i = 0; i < 7; ++i)
		WriteIO(&disp.io_seg[i], IO_High);
}


void DisplayEnable(bool state)
{
	disp.enabled = state;
	disp.str_mode = FALSE;
	if (disp.enabled)
		Display_On();
	else
		Display_Off();
}


void Display_SetBrightnessPWM(uint16_t pwm_period, uint8_t percent)
{
	if (percent > 99)
		percent = 99;
	else if (percent < 1)
		percent = 1;
		
	disp.time_on = (uint16_t)((((uint32_t)pwm_period) * percent) / 100);
	disp.time_off = pwm_period - disp.time_on;
}


void Display_StringSet(
	uint8_t *str,
	uint16_t t_char, uint16_t t_space, uint16_t t_end,
	uint16_t upd_period, bool loop
)
{
	uint8_t i;
	
	if (!disp.enabled)
		return;
		
	disp.str_pointer = str;
	disp.str_char_time = (uint8_t)(t_char / upd_period);
	disp.str_space_time = (uint8_t)(t_space / upd_period);
	disp.str_end_time = (uint8_t)(t_end / upd_period);
	disp.str_loop = loop;
	disp.str_cur_pos = 0;
	disp.str_cur_time = disp.str_char_time;
	disp.cur_symbol = disp.str_pointer[disp.str_cur_pos];
	disp.str_mode = TRUE;
}


void Display_StringUpdate(void)
{
	if ( (!disp.str_mode) || (!disp.enabled) )
		return;
		
	--disp.str_cur_time;
	if (disp.str_cur_time == 0) {
		if (disp.str_cur_pos & 0x80) {
du_show_symbol:
			disp.str_cur_time = disp.str_char_time;
			disp.str_cur_pos = (disp.str_cur_pos & 0x7F) + 1;
			if (disp.str_pointer[disp.str_cur_pos] == DS_NULL) {
				if (disp.str_loop) {
					disp.str_cur_pos = 0;
				} else {
					Display_SetSymbol(DS_NONE);
					return;
				}
			}
			disp.cur_symbol = disp.str_pointer[disp.str_cur_pos];
		} else { 
			disp.str_cur_time = (disp.str_pointer[disp.str_cur_pos + 1] == DS_NULL) ? disp.str_end_time : disp.str_space_time;
			if (disp.str_cur_time == 0)
				goto du_show_symbol;
			disp.str_cur_pos |= 0x80;
			disp.cur_symbol = DS_NONE;
		}
	}
}


bool Display_StringIsActive(void)
{
	return disp.str_mode;
}


void Display_WriteDot(IO_MODE_TypeDef mode)
{
	WriteIO(&disp.io_seg[7], mode);
}


DispSymbol_TypeDef Display_GetCurSymbol(void)
{
	return ( (disp.enabled) ? disp.cur_symbol : DS_NONE);
}


void Display_EnableFlash(bool state)
{
	IO_MODE_TypeDef mode;
	
	disp.flash_enabled = state;
	mode = state ? IO_High : IO_Low;
	WriteIO(&disp.io_flash, mode);
}


bool Display_IsEnabled(void)
{
	return disp.enabled;
}


bool Display_FlashIsEnabled(void)
{
	return disp.flash_enabled;
}
