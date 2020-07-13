/********************************************************************************************
* 	 	File: 		screen.h																*
*		Version:    1.1.1                                           						*
*      	Date: 		April 1st, 2020	                                    					*
*      	Author: 	Mogens Groth Nicolaisen                                					*
*                                                   										*	
*********************************************************************************************
* (C) 2020                                                                                  *
*                                                                                           *
* uStepper ApS                                                                              *
* www.ustepper.com                                                                          *
* administration@ustepper.com                                                               *
*                                                                                           *
* The code contained in this file is released under the following open source license:      *
*                                                                                           *
*     Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International               *
*                                                                                           *
*   The code in this file is provided without warranty of any kind - use at own risk!       *
*   neither uStepper ApS nor the author, can be held responsible for any damage             *
*   caused by the use of the code contained in this file !                                  *
*                                                                                           *
********************************************************************************************/
/**
 * @file screen.cpp
 * @brief      class implementation for the screen handling
 *
 * @author     Mogens Groth Nicolaisen (mogens@ustepper.com)
 */
#include "screen.h"
#include "egoShieldS.h"

Screen::Screen(bool i2cChannel)
{
	if(i2cChannel)
	{
		// activate internal pullups for twi.
		digitalWrite(SDA0, HIGH);
		digitalWrite(SCL0, HIGH);
	}
	else
	{
		// activate internal pullups for twi.
		digitalWrite(SDA1, HIGH);
		digitalWrite(SCL1, HIGH);
	}
	
	if(i2cChannel)
	{
		this->twsr = 0xD9;
		this->twbr = 0xD8;
		this->twdr = 0xDB;
		this->twcr = 0xDC;	
	}
	else
	{
		this->twsr = 0xB9;
		this->twbr = 0xB8;
		this->twdr = 0xBB;
		this->twcr = 0xBC;	
	}
	
}
void Screen::init()
{
	
	if(this->busFailure)
	{
		_SFR_MEM8(this->twcr) = (1 << TWEN0)| (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
		
	}
	this->busFailure = 0;

	//reset bus
	_SFR_MEM8(this->twsr) = 0;
	_SFR_MEM8(this->twcr) = 0;

	// set bit rate register to 72 to obtain 100kHz scl frequency (in combination with no prescaling!)
	_SFR_MEM8(this->twbr) = 72;
	// no prescaler
	_SFR_MEM8(this->twsr) &= 0xFC;
	// enable twi module, acks, and twi interrupt
	_SFR_MEM8(this->twcr) = _BV(TWEN0);

	SSD1306_SEND_CMD(SSD1306_DISPLAY_OFF);
    SSD1306_SEND_CMD(SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO);
    SSD1306_SEND_CMD(0x80);
    SSD1306_SEND_CMD(SSD1306_SET_MULTIPLEX_RATIO);
    SSD1306_SEND_CMD(H64_MULTIPLEX_RATIO);			// 0x3F for 128x64, 0x1F for 128x32
    SSD1306_SEND_CMD(SSD1306_SET_DISPLAY_OFFSET);
    SSD1306_SEND_CMD(0x0);
    SSD1306_SEND_CMD(SSD1306_SET_START_LINE | 0x0);
    SSD1306_SEND_CMD(SSD1306_CHARGE_PUMP);
	SSD1306_SEND_CMD(0x14);
    SSD1306_SEND_CMD(SSD1306_MEMORY_ADDR_MODE);
    SSD1306_SEND_CMD(0x00);
    SSD1306_SEND_CMD(SSD1306_SET_SEGMENT_REMAP | 0x1);
    SSD1306_SEND_CMD(SSD1306_COM_SCAN_DIR_DEC);
    SSD1306_SEND_CMD(SSD1306_SET_COM_PINS);
    SSD1306_SEND_CMD(H64_COM_PINS);			// 0x12 for 128x64, 0x02 for 128x32
    SSD1306_SEND_CMD(SSD1306_SET_CONTRAST_CONTROL);
	SSD1306_SEND_CMD(0xCF);
    SSD1306_SEND_CMD(SSD1306_SET_PRECHARGE_PERIOD);
	SSD1306_SEND_CMD(0xF1);
    SSD1306_SEND_CMD(SSD1306_SET_VCOM_DESELECT);
    SSD1306_SEND_CMD(0x40);
    SSD1306_SEND_CMD(SSD1306_DISPLAY_ALL_ON_RESUME);
    SSD1306_SEND_CMD(SSD1306_NORMAL_DISPLAY);
	SSD1306_SEND_CMD(SSD1306_DISPLAY_ON);

	clrScreen();
}

void* Screen::operator new(size_t size)
{
	void *object = malloc(size);
	return object;
}

bool Screen::waitForAck()
{
	int32_t time = millis();

	while ((_SFR_MEM8(this->twcr) & (1 << TWINT0)) == 0)
	{
		if(millis() - time > 10)
		{
			return 1;
		}	
	};
	return 0;
}

void Screen::clrScreen()
{
	
	if(this->busFailure)
	{
		return;
	}
	
	SSD1306_SEND_CMD(SSD1306_SET_COLUMN_ADDR);
	SSD1306_SEND_CMD(0);
	SSD1306_SEND_CMD(127);

	SSD1306_SEND_CMD(SSD1306_SET_PAGE_ADDR);
	SSD1306_SEND_CMD(0);
	SSD1306_SEND_CMD(7);

	// Send start address
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWEA0) | (1 << TWINT0) | (1 << TWSTA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_ADDR<<1;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_DATA_CONTINUE;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}

	for (uint16_t b=0; b < 128*8; b++)		// Send data
	{
		_SFR_MEM8(this->twdr) = 0x00;
		_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
		if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
	}

	_SFR_MEM8(this->twcr) = (1 << TWEN0)| (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
	 
}

void Screen::drawImage(const uint8_t *image, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
	int16_t i;
	uint8_t pattern;

	if(this->busFailure)
	{
		return;
	}
	
	SSD1306_SEND_CMD(SSD1306_SET_COLUMN_ADDR);
	SSD1306_SEND_CMD(x);
	SSD1306_SEND_CMD(x+width-1);

	SSD1306_SEND_CMD(SSD1306_SET_PAGE_ADDR);
	SSD1306_SEND_CMD(y/8);
	SSD1306_SEND_CMD(y/8 + height/8 - 1);

	// Send start address
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWEA0) | (1 << TWINT0) | (1 << TWSTA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_ADDR<<1;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_DATA_CONTINUE;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	for(i = (width*(height/8))-1; i > 0; i--)
	{
		pattern = pgm_read_byte(&(image[i]));

		if(invert)
		{
			pattern ^= 0xFF;
		}

		_SFR_MEM8(this->twdr) = pattern;
			
		_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
		if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
	}

	_SFR_MEM8(this->twcr) = (1 << TWEN0)| (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
	 
}

void Screen::printString(const uint8_t *string, uint8_t x, uint8_t y, bool invert)
{
	if(this->busFailure)
	{
		return;
	}

	const uint8_t *usedFont = font;
	uint16_t i, j, len, k;
	uint8_t pattern, width, height, rows = pgm_read_byte(&(usedFont[0])), offset = pgm_read_byte(&(usedFont[2]));
	uint8_t patternOffset, runs;

	len = strlen(string);

	width = len*rows;
	height = width/128;
	if(width > 128)
	{
		width = 128;
	}


	
	SSD1306_SEND_CMD(SSD1306_SET_COLUMN_ADDR);
	SSD1306_SEND_CMD(x);
	SSD1306_SEND_CMD(x+width-1);

	SSD1306_SEND_CMD(SSD1306_SET_PAGE_ADDR);
	SSD1306_SEND_CMD(y/8);
	SSD1306_SEND_CMD(y/8+height+runs-1);

	// Send start address
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWEA0) | (1 << TWINT0) | (1 << TWSTA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_ADDR<<1;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_DATA_CONTINUE;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}

		for(i = 0; i < len; i++)
		{
			for(j = 0; j < rows; j++)
			{
				pattern = pgm_read_byte(&(usedFont[((string[i]-offset)*rows) + 4 + j]));

				if(invert)
				{
					pattern ^= 0xFF;
				}

				_SFR_MEM8(this->twdr) = pattern;
					
				_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
				if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
			}
		}
	

	_SFR_MEM8(this->twcr) = (1 << TWEN0)| (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
	 
}

void Screen::drawRect(int x1, int y1, int x2, int y2, bool color)
{
	uint16_t b = 0;
	uint8_t pattern;
	uint8_t mask;

	if(this->busFailure)
	{
		return;
	}

	if(y2 < y1 || x2 < x1)
	{
		return;
	}
	if(x1 < 0 || x2 > 127 || y1 < 0 || y2 > 63)
	{
		return;
	}

	
	SSD1306_SEND_CMD(SSD1306_SET_COLUMN_ADDR);
	SSD1306_SEND_CMD(x1);
	SSD1306_SEND_CMD(x2);

	SSD1306_SEND_CMD(SSD1306_SET_PAGE_ADDR);
	SSD1306_SEND_CMD(y1/8);
	SSD1306_SEND_CMD(y2/8);

	// Send start address
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWEA0) | (1 << TWINT0) | (1 << TWSTA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_ADDR<<1;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}
	_SFR_MEM8(this->twdr) = SSD1306_DATA_CONTINUE;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);
	if(this->waitForAck()){this->busFailure = 1;  return;}

	if(color)
	{
		pattern = 0xFF;
	}
	else
	{	
		pattern = 0x00;
	}
	mask = 0x01;	
	for(b = y1%8; b > 0; b--)
	{
		pattern ^= mask;
		mask <<= 1;
	}

	if(y2/8 == y1/8)
	{
		mask = 0x80;	
		for(b = 7 - y2%8; b > 0; b--)
		{
			pattern ^= mask;
			mask >>= 1;
		}
	}
	
	for (b=0; b < (x2 - x1)+1; b++)		// Send data
	{
		_SFR_MEM8(this->twdr) = pattern;

		_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
		if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
	}

	if(y2/8 != y1/8)
	{
		if((y2/8 - y1/8) > 1)
		{
			if(color)
			{
				pattern = 0xFF;
			}
			else
			{	
				pattern = 0x00;
			}
			for (b=0; b < (((y2/8) - (y1/8)) - 1) * ((x2 - x1) + 1); b++)		// Send data
			{
				_SFR_MEM8(this->twdr) = pattern;

				_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
				if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
			}
		}
		
		if(color)
		{
			pattern = 0x00;
		}
		else
		{	
			pattern = 0xFF;
		}
		mask = 0x01;	
		for(b = y2%8 + 1; b > 0; b--)
		{
			pattern ^= mask;
			mask <<= 1;
		}
		for (b=0; b < (x2 - x1)+1; b++)		// Send data
		{
			_SFR_MEM8(this->twdr) = pattern;
			
			_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
			if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
		}
	}
	

	_SFR_MEM8(this->twcr) = (1 << TWEN0)| (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
	 
	
}

void Screen::cmd(uint8_t cmd)
{
	if(this->busFailure)
	{
		return;
	}
	
	// Send start address
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWEA0) | (1 << TWINT0) | (1 << TWSTA0);						// Send START
	if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
	_SFR_MEM8(this->twdr) = SSD1306_ADDR<<1;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
	if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready

	_SFR_MEM8(this->twdr) = SSD1306_COMMAND;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
	if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready
	_SFR_MEM8(this->twdr) = cmd;
	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWEA0);									// Clear TWINT to proceed
	if(this->waitForAck()){this->busFailure = 1;  return;}											// Wait for TWI to be ready

	_SFR_MEM8(this->twcr) = (1 << TWEN0) | (1 << TWINT0) | (1 << TWSTO0);									// Send STOP
	 
}