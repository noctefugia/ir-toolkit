#include "button.h" 

struct button_struct btn = {0};


void Button_Init(
	uint16_t upd_period, uint16_t rld_time,
	void (*click_evt)(uint8_t)
)
{
	uint8_t i;
	
	btn.click_event = click_evt;
	btn.reload_time = rld_time / upd_period;
	
	Button_ResetIO();
	for (i = 0; i < BUTTON_COUNT; ++i) {
		btn.id[i].state = FALSE;
		btn.id[i].cur_time = 0;
		btn.id[i].hold_time = 0;
	}
	btn.enabled = TRUE;
}


void Button_ResetIO(void)
{
	AssignIO(&btn.id[0].io, PORT_BTN_A, PIN_BTN_A, TRUE, GPIO_MODE_IN_PU_NO_IT);
	AssignIO(&btn.id[1].io, PORT_BTN_B, PIN_BTN_B, TRUE, GPIO_MODE_IN_PU_NO_IT);
	AssignIO(&btn.id[2].io, PORT_BTN_C, PIN_BTN_C, TRUE, GPIO_MODE_IN_PU_NO_IT);
}


void Button_Update(void)
{
	uint8_t i;
		
	if (!btn.enabled)
		return;
		
	for (i = 0; i < BUTTON_COUNT; ++i) {
		btn.id[i].state = GPIO_ReadInputPin(btn.id[i].io.port, btn.id[i].io.pin);
		if (btn.id[i].state == LOW) {
			if (btn.id[i].cur_time == 0) {
				(*btn.click_event)(i);
				btn.id[i].cur_time = btn.reload_time;
				btn.id[i].hold_time = 1;
			} else if (btn.id[i].hold_time < 0xFF) {
				++btn.id[i].hold_time;
			}
		} else if ( (btn.id[i].cur_time > 0) && (btn.id[i].state != LOW) ) {
			--btn.id[i].cur_time;
			btn.id[i].hold_time = 0;
		}
	}
}


uint16_t Button_IsDown(uint16_t upd_period, uint8_t index)
{
	return ( (btn.id[index].state == LOW) ? (uint16_t)(upd_period * btn.id[index].hold_time) : 0 );
}

void Button_Enable(bool state)
{
	btn.enabled = state;
}