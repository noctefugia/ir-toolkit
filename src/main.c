#include "utils.h"
#include "display.h"
#include "button.h"

#define SYS_UPDATE_PERIOD 50
#define TIMER2_PERIOD 1024
#define SYS_BUTTON_RELOAD_TIME 100
#define SYS_FLASHLIGHT_POWERSAFE_TIME 15000 //15sec
#define SYS_LOCKSCREEN_SHUTDOWN_TIME 10000 //10sec
#define SYS_SHUTDOWN_TIME 300000 //5min
#define SYS_SHUTDOWN_BTN_HOLD_TIME 1500 //3sec
#define SYS_IR_BUFF_SIZE 70
#define SYS_IR_EEPROM_DATA_BEGIN 0x4005
#define SYS_NORMAL_POWER_BRIGHTNESS 80
#define SYS_LOW_POWER_BRIGHTNESS 25
#define SYS_LOW_POWER_BLINK_PERIOD 2500
#define SYS_WWDG_MIN_VALUE 0x5A
#define SYS_WWDG_INIT_VALUE 0x6E
#define SYS_RESET_FLAG_ADDR 0x4000

typedef enum {
	SS_LOCKSCREEN,
	SS_MAIN_MENU,
	SS_FLASHLIGHT,
	SS_IR_REMOTE,
	SS_IR_FLOOD,
	SS_POWERBANK,
	SS_INVALID = 0xFF
} SystemState_TypeDef;

typedef enum {
	SBTN_UP,
	SBTN_DOWN,
	SBTN_OK,
	SBTN_UNDEFINED = 0xFF
} SystemButton_TypeDef;
#define SYS_UNLOCK_BUTTON SBTN_OK

struct system_struct {
	struct io_struct io_ain_vmon, io_ir_rxp_usbm, io_ir_rxi, io_powerbank;
	uint32_t cpu_freq, counter;
	bool normal_power, shutdown, ir_recording, ir_rec_flag, ir_tim_flag, ir_replay, ir_test;
	SystemState_TypeDef state;
	uint8_t brightness, *buff_pointer;
	uint16_t idle_time;
};
struct system_struct sys = {0};
//extern struct system_struct sys;


void Clock_Init(void);
void Timer_Init(uint32_t period_ms);
void Timer_Interrupt(void);
void Timer2_Interrupt(void);
void Timer3_Interrupt(void);
void SleepMs(uint32_t time_ms);
void Event_ButtonClick(uint8_t index);
void Lockscreen(void);
void RecordIR(void);
void Pin_Interrupt(void);
void PlayIR(uint8_t index, uint8_t* buffer);
void BeginIR(void);
void ResetWatchdog(void);
uint8_t SetResetFlag(uint8_t value);

main()
{
	while ( GPIO_ReadInputPin(PORT_BTN_C, PIN_BTN_C) == LOW)
		;
		
	WWDG_Init(SYS_WWDG_INIT_VALUE, SYS_WWDG_MIN_VALUE);

	disableInterrupts();
	ITC_DeInit(); EXTI_DeInit();
	GPIO_DeInit(GPIOA); GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC); GPIO_DeInit(GPIOD);
	Clock_Init();
	Sleep(5000);
	
	AssignIO(&sys.io_ain_vmon, PORT_AUDIO_IN_VMON, PIN_AUDIO_IN_VMON, TRUE, GPIO_MODE_IN_FL_NO_IT);
	AssignIO(&sys.io_ir_rxp_usbm, PORT_IR_RX_PWR_USBM, PIN_IR_RX_PWR_USBM, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	AssignIO(&sys.io_ir_rxi, PORT_IR_RX_IN, PIN_IR_RX_IN, TRUE, GPIO_MODE_IN_FL_NO_IT);
	AssignIO(&sys.io_powerbank, PORT_POWERBANK, PIN_POWERBANK, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	Timer_Init(SYS_UPDATE_PERIOD);
	Button_Init(SYS_UPDATE_PERIOD, SYS_BUTTON_RELOAD_TIME, &Event_ButtonClick);
	sys.brightness = SYS_NORMAL_POWER_BRIGHTNESS;
	Display_Init(TRUE, TIMER2_PERIOD, sys.brightness, DS_NONE);
	
	sys.counter = 0;
	sys.idle_time = 0;
	sys.normal_power = TRUE;
	sys.shutdown = FALSE;
	sys.ir_recording = FALSE;
	sys.ir_rec_flag = FALSE;
	sys.ir_tim_flag = FALSE;
	sys.state = SS_LOCKSCREEN;
	sys.buff_pointer = NULL;
	sys.ir_replay = FALSE;
	sys.ir_test = FALSE;
	enableInterrupts();
	//SleepMs(100);
	//while ( Button_IsDown(SYS_UPDATE_PERIOD, SYS_UNLOCK_BUTTON) )
		//ResetWatchdog();
	
	if (FLASH_ReadByte(SYS_RESET_FLAG_ADDR) == 1)
		Display_StringSet(DISP_STR_STARTUP, 500, 0, 500, SYS_UPDATE_PERIOD, TRUE);
	else
		Display_StringSet(DISP_STR_HELLO, 600, 100, 900, SYS_UPDATE_PERIOD, TRUE);
	SetResetFlag(0);
	
	Lockscreen();
	WriteIO(&sys.io_ir_rxp_usbm, IO_High);
	Display_SetSymbol(DS_1);
	
	//SleepMs(4200);
	//Display_StringSet(DISP_STR_USB, 600, 100, 900, SYS_UPDATE_PERIOD, TRUE);
	//Display_StringSet(DISP_STR_LOADING, 100, 0, 0, SYS_UPDATE_PERIOD, TRUE);
	//Display_StringSet(DISP_STR_CHARGING, 500, 0, 500, SYS_UPDATE_PERIOD, TRUE);
	
	while (!sys.shutdown) {
		ResetWatchdog();
		if (sys.state == SS_IR_FLOOD) {
			Display_WriteDot(IO_Low);
			Sleep(225);
			Display_WriteDot(IO_High);
			Sleep(225);
			
			Display_WriteDot(IO_Low);
			Sleep(225);
			Display_WriteDot(IO_High);
			Sleep(225*3);
		} else if (sys.ir_recording) {
			RecordIR();
		} else if (sys.ir_test) {
			Display_WriteDot(IO_Low);
			while (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_OK))
				ResetWatchdog();
			while (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_OK) == 0)
				ResetWatchdog();
			Display_WriteDot(IO_High);
			sys.ir_test = FALSE;
		}
		
		else if ( (!sys.normal_power) \
		&& ((sys.idle_time % (SYS_LOW_POWER_BLINK_PERIOD / SYS_UPDATE_PERIOD)) == 0) ) {
			Button_Enable(FALSE);
			GPIO_Init(PORT_BTN_A, PIN_BTN_A, GPIO_MODE_OUT_PP_LOW_SLOW);
			GPIO_Init(PORT_BTN_B, PIN_BTN_B, GPIO_MODE_OUT_PP_LOW_SLOW);
			SleepMs(50);
			Button_ResetIO();
			Button_Enable(TRUE);
		}
	}
	
	Display_SetSymbol(DS_NONE);
	Display_EnableFlash(FALSE);
	while ( Button_IsDown(SYS_UPDATE_PERIOD, SYS_UNLOCK_BUTTON) )
		ResetWatchdog();
	SleepMs(100);
	GPIO_DeInit(GPIOA); GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC); GPIO_DeInit(GPIOD);
	GPIO_Init(PORT_BTN_C, PIN_BTN_C, GPIO_MODE_IN_PU_IT);
	disableInterrupts();
	TIM1_Cmd(DISABLE); TIM2_Cmd(DISABLE); //TIM4_Cmd(DISABLE);
	ITC_DeInit();
	ITC_SetSoftwarePriority(ITC_IRQ_PORT_BTN_C, ITC_PRIORITYLEVEL_0);
	EXTI_DeInit();
	EXTI_SetExtIntSensitivity(EXTI_PORT_BTN_C, EXTI_SENSITIVITY_FALL_ONLY);
	EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);
	SetResetFlag(1);
	ResetWatchdog();
	enableInterrupts();
	halt();
	WWDG_SWReset();
}


uint8_t SetResetFlag(uint8_t value)
{
	FLASH_Unlock(FLASH_MEMTYPE_DATA);
	FLASH_ProgramByte(SYS_RESET_FLAG_ADDR, value);
	FLASH_Lock(FLASH_MEMTYPE_DATA);
}


void ResetWatchdog(void)
{
	if ((WWDG_GetCounter() & 0x7F) <= SYS_WWDG_MIN_VALUE)
		WWDG_SetCounter(SYS_WWDG_INIT_VALUE);
}

void RecordIR(void)
{
	uint8_t i, raw_data[SYS_IR_BUFF_SIZE];
	uint32_t temp;
	bool state;
	DispSymbol_TypeDef last_symbol;
	
	last_symbol = Display_GetCurSymbol();
	if ( (last_symbol >= DS_1) && (last_symbol <= DS_9) ) {
		for (i = 0; i < SYS_IR_BUFF_SIZE+1; ++i)
			raw_data[i] = 0x00;
		raw_data[SYS_IR_BUFF_SIZE-1] = 0xFF;
		sys.buff_pointer = &raw_data[0];
		
		while (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_UP))
			ResetWatchdog();
		while (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_DOWN))
			ResetWatchdog();
		
		Display_SetSymbol(DS_SEG_G);
		BeginIR();
		
		sys.ir_rec_flag = TRUE;
		sys.ir_tim_flag = FALSE;
		ModeIO(&sys.io_ir_rxi, GPIO_MODE_IN_FL_IT);
		ITC_DeInit();
		ITC_SetSoftwarePriority(ITC_IRQ_PORT_IR_IN, ITC_PRIORITYLEVEL_0);
		EXTI_DeInit();
		EXTI_SetExtIntSensitivity(EXTI_PORT_IR_IN, EXTI_SENSITIVITY_RISE_FALL);
		EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);
		enableInterrupts();
		
		state = TRUE;
		while (sys.ir_rec_flag) {
			ResetWatchdog();
			if (GPIO_ReadInputPin(PORT_BTN_C, PIN_BTN_C) == LOW) {
				state = FALSE;
				break;
			}
		}
		
		TIM1_Cmd(DISABLE);
		sys.ir_rec_flag = FALSE;
		EXTI_DeInit();
		ITC_DeInit();
		ModeIO(&sys.io_ir_rxi, GPIO_MODE_IN_FL_NO_IT);
		
		if (state) {
			state = TRUE;
			//Sleep(150000);
			FLASH_Unlock(FLASH_MEMTYPE_DATA);
			for (i = 0; i < SYS_IR_BUFF_SIZE; ++i) {
				ResetWatchdog();
				FLASH_ProgramByte(SYS_IR_EEPROM_DATA_BEGIN+SYS_IR_BUFF_SIZE*(last_symbol-1)+i, raw_data[i]);
			} FLASH_Lock(FLASH_MEMTYPE_DATA);
			
			for (i = 0; i < SYS_IR_BUFF_SIZE; ++i) {
				ResetWatchdog();
				if ( ((temp = raw_data[i]) == 0) || (temp == 0xFF) )
					break;
				Display_EnableFlash(state);
				state = !state;
				Sleep(20 * temp);
			}
			Display_EnableFlash(FALSE);
		}
		
		Timer_Init(SYS_UPDATE_PERIOD);
		Display_SetSymbol(last_symbol);
		DisplayEnable(TRUE);
		while (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_OK))
			ResetWatchdog();
	}
	sys.ir_recording = FALSE;
}


void BeginIR(void)
{
	disableInterrupts();
	DisplayEnable(TRUE);
	TIM2_Cmd(DISABLE);
	TIM1_DeInit();
	TIM1_TimeBaseInit(160, TIM1_COUNTERMODE_UP, 3, 0); //160-4
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
}


void PlayIR(uint8_t index, uint8_t* buffer)
{
	uint8_t i, ir_data[SYS_IR_BUFF_SIZE];
	uint32_t t;
	
	if ( (index > DS_9) || (index < DS_1) )
		return;
		
	BeginIR();
	
	if (buffer == NULL) {
		//FLASH_Unlock(FLASH_MEMTYPE_DATA);
		for (i = 0; i < SYS_IR_BUFF_SIZE; ++i)
			ir_data[i] = FLASH_ReadByte(SYS_IR_EEPROM_DATA_BEGIN+SYS_IR_BUFF_SIZE*(index-1)+i);
		//FLASH_Lock(FLASH_MEMTYPE_DATA);
	}
	
	sys.buff_pointer = (buffer == NULL) ? &ir_data[0] : &buffer[0];
	sys.ir_replay = TRUE;
	
	Display_WriteDot(IO_Low);
	enableInterrupts();
	TIM1_Cmd(ENABLE);
	while (sys.ir_replay)
		ResetWatchdog();
	Display_WriteDot(IO_High);
	
	Timer_Init(SYS_UPDATE_PERIOD);
}


void Pin_Interrupt(void)
{
	if (!sys.ir_rec_flag)
		return;
		
	if (!sys.ir_tim_flag) {
		TIM1_Cmd(ENABLE);
		sys.ir_tim_flag = TRUE;
	} else {
		++sys.buff_pointer;
		if ((*sys.buff_pointer) == 0xFF)
			sys.ir_rec_flag = FALSE;
	}
}


void Lockscreen(void)
{
	while (TRUE) {
		while (Button_IsDown(SYS_UPDATE_PERIOD, SYS_UNLOCK_BUTTON) == 0) {
			ResetWatchdog();
			if (sys.shutdown)
				return;
			else if ( (Display_GetCurSymbol() != DS_LOCK) && (!Display_StringIsActive()) )
				Display_SetSymbol(DS_LOCK);
		}
			
		while (Button_IsDown(SYS_UPDATE_PERIOD, SYS_UNLOCK_BUTTON)) {
			ResetWatchdog();
			if (!Display_StringIsActive())
				Display_SetSymbol(DS_LOCK);
		}

		if (Display_GetCurSymbol() == DS_0) {
			sys.state = SS_MAIN_MENU;
			Display_StringSet(DISP_STR_UNLOCKED, 250, 250, 350, SYS_UPDATE_PERIOD, FALSE);
			while (Display_StringIsActive())
				ResetWatchdog();
			return;
		} else if (Display_StringIsActive()) {
			Display_SetSymbol(DS_LOCK);
		}
	}
}


void Event_ButtonClick(uint8_t index)
{
	DispSymbol_TypeDef cur_symbol;
	bool cur_state;
	
	sys.idle_time = 0;
	if (!Display_IsEnabled()) {
		DisplayEnable(TRUE);
		if (sys.state == SS_IR_FLOOD)
			Display_StringSet(DISP_STR_FLOOD, 600, 100, 900, SYS_UPDATE_PERIOD, TRUE);
		return;
	}
			
	switch (sys.state) {
		
		case SS_LOCKSCREEN:
			if (index == SYS_UNLOCK_BUTTON)
				Display_StringSet(DISP_STR_UNLOCKING, 250, 0, 0, SYS_UPDATE_PERIOD, FALSE);
			break;
			
		case SS_MAIN_MENU:
			if (Display_StringIsActive())
				return;
			cur_symbol = Display_GetCurSymbol();
			switch (index) {
				case SBTN_OK:
					switch (cur_symbol) {
						case DS_1:
							Display_EnableFlash(TRUE);
							sys.brightness = 80;
							Display_SetBrightnessPWM(TIMER2_PERIOD, sys.brightness);
							Display_SetSymbol(DS_7);
							sys.state = SS_FLASHLIGHT;
							break;
						case DS_2:
							sys.state = SS_IR_REMOTE;
							Display_SetSymbol(DS_1);
							break;
						case DS_3:
							sys.state = SS_IR_FLOOD;
							Display_StringSet(DISP_STR_FLOOD, 600, 100, 900, SYS_UPDATE_PERIOD, TRUE);
							break;
						case DS_4:
							sys.state = SS_POWERBANK;
							Display_SetSymbol(DS_L);
							break;
					}
					break;
				case SBTN_UP:
					Display_SetSymbol( (cur_symbol == DS_4) ? DS_1 : ++cur_symbol );
					break;
				case SBTN_DOWN:
					Display_SetSymbol( (cur_symbol == DS_1) ? DS_4 : --cur_symbol );
					break;
			}
			break;
			
		case SS_FLASHLIGHT:
			switch (index) {
				case SBTN_OK:
					Display_EnableFlash(!Display_FlashIsEnabled());
					break;
				case SBTN_UP:
					if (sys.brightness < 100)
						sys.brightness += 10;
					break;
				case SBTN_DOWN:
					if (sys.brightness > 10)
						sys.brightness -= 10;
				break;
			}
			Display_SetBrightnessPWM(TIMER2_PERIOD, sys.brightness);
			Display_SetSymbol(DS_0 + sys.brightness/10 - 1);
			break;
			
		case SS_IR_REMOTE:
			if ( (sys.ir_recording) || (sys.ir_replay) || (sys.ir_test) )
				break;
			cur_symbol = Display_GetCurSymbol();
			switch (index) {
				case SBTN_OK:
					if (cur_symbol == DS_0)
						sys.ir_test = TRUE;
					else
						PlayIR(cur_symbol, NULL);
					break;
				case SBTN_UP:
					if (cur_symbol < DS_9)
						++cur_symbol;
					else
						cur_symbol = DS_0;
					break;
				case SBTN_DOWN:
					if (cur_symbol > DS_0)
						--cur_symbol;
					else
						cur_symbol = DS_9;
					break;
			}
			Display_SetSymbol(cur_symbol);
			break;
			
		case SS_IR_FLOOD:
			break;
			
		case SS_POWERBANK:
			switch (index) {
				case SBTN_OK:
					ModeIO(&sys.io_ir_rxp_usbm, GPIO_MODE_IN_FL_NO_IT);
					ReadIO(&sys.io_ir_rxp_usbm, &cur_state);
					ModeIO(&sys.io_ir_rxp_usbm, GPIO_MODE_OUT_PP_LOW_SLOW);
					
					cur_symbol = (Display_GetCurSymbol() == DS_H) ? DS_L : DS_H;
					WriteIO(&sys.io_powerbank, (cur_symbol == DS_H) ? IO_High : IO_Low);
					Display_SetSymbol(cur_symbol);
					
					break;
			}
			break;
	}
}


void Clock_Init(void)
{
	CLK_DeInit();
								 
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == FALSE)
		ResetWatchdog();
	CLK_HSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE)
		ResetWatchdog();
	
	CLK_ClockSwitchCmd(ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
	
	CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, 
	DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
	
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_AWU, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
	
	sys.cpu_freq = GetCurClockFreq();
}


void Timer_Init(uint32_t period_ms)
{
	uint16_t tim_period;
	
	TIM2_DeInit();
	tim_period = (uint16_t)(sys.cpu_freq / (1024 * (1000 / period_ms)));
	TIM2_TimeBaseInit(TIM2_PRESCALER_1024, tim_period); 
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
	TIM2_Cmd(ENABLE);
	
	TIM1_DeInit();
	TIM1_TimeBaseInit(256, TIM1_COUNTERMODE_UP, TIMER2_PERIOD, 0);
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_Cmd(ENABLE);
	
	//TIM4_DeInit();
	//TIM4_TimeBaseInit(TIM4_PRESCALER_128, 1);
	//TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
	//TIM4_Cmd(ENABLE);
}


void Timer_Interrupt(void)
{
	++sys.counter;
	if (sys.idle_time != 0xFFFF)
		++sys.idle_time;
		
	if ( (sys.idle_time > SYS_SHUTDOWN_TIME / SYS_UPDATE_PERIOD) && (sys.state =! SS_POWERBANK) )
		sys.shutdown = TRUE;
	else if ( (Button_IsDown(SYS_UPDATE_PERIOD, SYS_UNLOCK_BUTTON) > SYS_SHUTDOWN_BTN_HOLD_TIME) \
	&& (sys.state != SS_LOCKSCREEN) )
		sys.shutdown = TRUE;
	else if ( (sys.state == SS_LOCKSCREEN) \
	&& (sys.idle_time > SYS_LOCKSCREEN_SHUTDOWN_TIME / SYS_UPDATE_PERIOD) )
		sys.shutdown = TRUE;
	else if ( (sys.state == SS_FLASHLIGHT) \
	&& (sys.idle_time > SYS_FLASHLIGHT_POWERSAFE_TIME / SYS_UPDATE_PERIOD) \
	&& (Display_FlashIsEnabled()) )
		DisplayEnable(FALSE);
	else if ( (sys.state == SS_IR_FLOOD) \
	&& (sys.idle_time > SYS_FLASHLIGHT_POWERSAFE_TIME / SYS_UPDATE_PERIOD) )
		DisplayEnable(FALSE);
	else if ( (sys.state == SS_IR_REMOTE) \
	&& (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_UP)) \
	&& (Button_IsDown(SYS_UPDATE_PERIOD, SBTN_DOWN)) )
		sys.ir_recording = TRUE;
	
	/* update code begin */
	ReadIO(&sys.io_ain_vmon, &sys.normal_power); //adc
	
	if (sys.state != SS_FLASHLIGHT) {
		if ( (!sys.normal_power) \
		|| (sys.idle_time > SYS_FLASHLIGHT_POWERSAFE_TIME / SYS_UPDATE_PERIOD) ) {
			if (sys.brightness != SYS_LOW_POWER_BRIGHTNESS) {
				sys.brightness = SYS_LOW_POWER_BRIGHTNESS;
				Display_SetBrightnessPWM(TIMER2_PERIOD, sys.brightness);
			}
		} else if ( (sys.brightness != SYS_NORMAL_POWER_BRIGHTNESS) \
		&& (sys.normal_power) ) {
			sys.brightness = SYS_NORMAL_POWER_BRIGHTNESS;
			Display_SetBrightnessPWM(TIMER2_PERIOD, sys.brightness);
		}
	}
		
		
	Button_Update();
	Display_StringUpdate();
	/* update code end */
	
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
}


void Timer2_Interrupt(void)
{
	uint16_t period;
	
	if (sys.ir_recording) {
		if ((*sys.buff_pointer) == 0xFF)
			sys.ir_rec_flag = FALSE;
		else
			++*sys.buff_pointer;
	} 
	
	else if (sys.ir_replay) {
		if ((--*sys.buff_pointer) == 0) {
			Display_WriteDot(IO_Reverse);
			++sys.buff_pointer;
			if ( (*sys.buff_pointer == 0xFF) || (*sys.buff_pointer == 0x00) )
				sys.ir_replay = FALSE;
		}
	}
	
	else {
		period = Display_UpdatePWM();
		TIM1_SetAutoreload(period);
	}
	
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);
}

void Timer3_Interrupt(void)
{
	//TIM4_ClearFlag(TIM4_FLAG_UPDATE);
}


void SleepMs(uint32_t time_ms)
{
	uint32_t last_time;
	
	last_time = sys.counter * SYS_UPDATE_PERIOD;
	while ((sys.counter*SYS_UPDATE_PERIOD - last_time) < time_ms)
		ResetWatchdog();
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
	DisplayEnable(TRUE);
	Display_SetSymbol(DS_E);
  while (1)
  {
		ResetWatchdog();
  }
}
#endif

/*
> .text, .const and .vector are ROM
> .bsct, .ubsct, .data and .bss are all RAM
> .info. and .debug are symbol tables that do not use target resources/memory.
*/
