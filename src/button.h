#ifndef __BUTTON_H
#define __BUTTON_H

#include "utils.h"


#define BUTTON_COUNT 3


struct button_id_struct {
	BitStatus state;
	uint16_t cur_time;
	uint8_t hold_time;
	struct io_struct io;
};

struct button_struct {
	struct button_id_struct id[BUTTON_COUNT];
	void (*click_event)(uint8_t);
	uint16_t reload_time;
	bool enabled;
};

void Button_Init(uint16_t upd_period, uint16_t rld_time, void (*click_evt)(uint8_t));
void Button_ResetIO(void);
void Button_Update(void);
void Button_Enable(bool state);
uint16_t Button_IsDown(uint16_t upd_period, uint8_t index);

#endif /* __BUTTON_H */