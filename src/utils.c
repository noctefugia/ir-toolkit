#include "utils.h"

static const uint8_t HSIDivFactor[4] = {1, 2, 4, 8};

uint32_t GetCurClockFreq(void)
{
  uint32_t clockfrequency = 0;
  CLK_Source_TypeDef clocksource = CLK_SOURCE_HSI;
  uint8_t tmp = 0, presc = 0;
  
  /* Get CLK source. */
  clocksource = (CLK_Source_TypeDef)CLK->CMSR;
  
  if (clocksource == CLK_SOURCE_HSI)
  {
    tmp = (uint8_t)(CLK->CKDIVR & CLK_CKDIVR_HSIDIV);
    tmp = (uint8_t)(tmp >> 3);
    presc = HSIDivFactor[tmp];
    clockfrequency = HSI_VALUE / presc;
  }
  else if ( clocksource == CLK_SOURCE_LSI)
  {
    clockfrequency = LSI_VALUE;
  }
  else
  {
    clockfrequency = HSE_VALUE;
  }
  
  return((uint32_t)clockfrequency);
}


void Sleep(uint32_t t) {
    while(--t);
}


void AssignIO(
	struct io_struct *cur_io, 
	GPIO_TypeDef *cur_port, GPIO_Pin_TypeDef cur_pin, 
	bool init_io, GPIO_Mode_TypeDef cur_mode
) 
{
	cur_io->port = cur_port;
	cur_io->pin = cur_pin;
	if (init_io)
		GPIO_Init(cur_io->port, cur_io->pin, cur_mode);
}

void WriteIO(struct io_struct *cur_io, IO_MODE_TypeDef io_mode) 
{
	assert_param(IS_UTIL_IO_MODE_OK(io_mode));
	
	switch (io_mode) {
		case IO_High:
			GPIO_WriteHigh(cur_io->port, cur_io->pin);
			break;
		case IO_Low:
			GPIO_WriteLow(cur_io->port, cur_io->pin);
			break;
		case IO_Reverse:
			GPIO_WriteReverse(cur_io->port, cur_io->pin);
			break;
	}
}


void ReadIO(struct io_struct *cur_io, bool* result) 
{
	BitStatus status;
	
	status = GPIO_ReadInputPin(cur_io->port, cur_io->pin);
	*result = (bool)(status ? TRUE : FALSE);
}


void ModeIO(struct io_struct *cur_io, GPIO_Mode_TypeDef cur_mode) 
{
	GPIO_Init(cur_io->port, cur_io->pin, cur_mode);
}


uint8_t DecFromBCD(uint8_t bcd_val)
{
	uint8_t dec_val;
	
	dec_val = (uint8_t)((bcd_val >> 4)*10 + (bcd_val & 0x0F));
	
	return dec_val;
}


uint8_t DecToBCD(uint8_t dec_val)
{
	uint8_t bcd_val;
	
	if (dec_val > 99)
		dec_val = 99;
		
	bcd_val = (uint8_t)(((dec_val / 10) << 4) + (dec_val % 10));
	
	return bcd_val;
}


//MaxLen: 4095
uint16_t Crc16(uint8_t *pcBlock, uint8_t len)
{
	uint16_t crc;
	uint8_t i;
	
	crc = 0xFFFF;
	while (len--)
	{
		crc ^= *pcBlock++ << 8;
		
		for (i = 0; i < 8; i++)
			crc = (uint16_t)(crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1);
	}
	return crc;
}


int8_t Round8S(float x)
{
    if (x < 0.0f)
        return (int8_t)(x - 0.5f);
    else
        return (int8_t)(x + 0.5f);
}


uint8_t Round8U(float x)
{
	return (uint8_t)(x + 0.5f);
}