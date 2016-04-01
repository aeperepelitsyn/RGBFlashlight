////////////////////////////////////////////////////////////////////////////////
// File       : RGBFlashlight.cpp
// Description: Main file of firmware for prototype of RGB Flashlight (20150630)
// Created    : 2015.06.21 19:45:19
// Author     : aeperepelitsyn
// Version    : 1.00 (2015.09.07 22:11:05)
////////////////////////////////////////////////////////////////////////////////

#include <avr/io.h>

#define F_CPU 1000000UL

#include <avr/power.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <stdlib.h>

#define FL_STUB_2015

	
#include "RGBFlashlight.h"


enum FLStates 
{
	sleeping, oversleep, charging, idle
};

//volatile unsigned char adcvalue = 0;

inline void SetDCDCResistor(unsigned char level)
{
	unsigned char resistors = DDRD & 0b00011111;
	switch(level)
	{
		case 0:
		{
			resistors |= 0b00000000;
			break;
		}
		case 1:
		{
			resistors |= 0b00100000;
			break;
		}
		case 2:
		{
			resistors |= 0b01000000;
			break;
		}
		case 3:
		{
			resistors |= 0b10000000;
			break;
		}
		case 4:
		{
			resistors |= 0b01100000;
			break;
		}
		case 5:
		{
			resistors |= 0b10100000;
			break;
		}
		case 6:
		{
			resistors |= 0b11000000;
			break;
		}
		case 7:
		{
			resistors |= 0b11100000;
			break;
		}
	}
	DDRD = resistors;
}

inline void SetBrightness(unsigned char level)
{
	if (level == 1)
	{
		PORTB &= ~(FL_RGBLED_MAIN_DISABLE);
		PORTB &= ~(FL_PWM_NOT_R | FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI);
	}
	else
	{
		PORTB |= (FL_RGBLED_MAIN_DISABLE);
		PORTB |= (FL_PWM_NOT_R | FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI);
	}
	
	if (level == 2)
	{
		PORTC |= (FL_ADC5_MAINLED_VOLTAGE);
		DDRC |= (FL_ADC5_MAINLED_VOLTAGE);
	}
	else
	{
		DDRC &= ~(FL_ADC5_MAINLED_VOLTAGE);
		PORTC &= ~(FL_ADC5_MAINLED_VOLTAGE);
	}
	
	if(level>= 3 && level<= 10)
	{
		PORTD |= (FL_MAINLED_DCDC_ENABLE);
		SetDCDCResistor(level - 3);
	}
	else
	{
		PORTD &= ~(FL_MAINLED_DCDC_ENABLE);
	}
}

int main(void)
{
	wdt_reset();
	// Ports initialization
	ResetPorts();
	

	unsigned char pwm = 0;
	unsigned char Green = 255;


	unsigned char wakefulness = 255;
	unsigned char brightness = 0;
	
	

	unsigned char switchingon = 0; // State
	unsigned char switchingin = 0; // Index
	
	// Current state of FSM
	FLStates state = oversleep;
	
	if(GetAdcValue(6) < 0x30 || GetAdcValue(7) < 0x30)
	{	// Charged or Charging
		state = charging;
	}
	else
	{
		for(char i=0; i<4; i++)
		{
			button_levels[i] = GetButtonDifference(i);
			if (button_levels[i] < 36)
			{
				state = sleeping;
				break;
			}
		}
	}
	
	unsigned char max_p = 0;
	unsigned char index_p = 4;
	unsigned char charge_counter = 0;
	
	wdt_enable(WDTO_2S);

	while(1)
	{
		switch(state)
		{
			case sleeping:
			{
				set_sleep_mode(SLEEP_MODE_PWR_DOWN);
				sleep_enable();
				sleep_cpu(); // Power Down
				break;
			}
			case oversleep:
			{
				wakefulness--;

				
				
				unsigned char max_button = GetButtonStatistics();
				DDRB &= ~(FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				PORTB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				PORTB &= ~(FL_PWM_NOT_G);

				if (max_button == 0)
				{
					switchingon = 0;
				}
				else
				{
					if(switchingon == 0)
					{
						switchingon = 0x0E;
						for(unsigned char i=0; i<4; i++)
						{
							if((max_button >> i) == 1)
							{
								switchingin = i;
								switchingon = 1;
								DDRB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
								break;
							}
							else if (((max_button >> i)&0x01) == 1)
							{
								break;
							}
						}
					}
					else if(switchingon < 5)
					{
						if (max_button != (1<<switchingin))
						{
							unsigned char current_switching = 4;
							switch (max_button)
							{
								case 1:
								{
									if(switchingin == 2) // Up
									{
										current_switching = 0;
									}
									break;
								}
								case 2:
								{
									if(switchingin == 0) // Right
									{
										current_switching = 1;
									}
									break;
								}
								case 4:
								{
									if(switchingin == 3) // Left
									{
										current_switching = 2;
									}
									break;
								}
								case 8:
								{
									if(switchingin == 1) // Down
									{
										current_switching = 3;
									}
									break;
								}
								default:
								{
									break;
								}
							}
							if(current_switching < 4)
							{
								wakefulness = 255;
								switchingin = current_switching;
								switchingon++;
								DDRB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
								if(switchingon == 5)
								{
									switchingin = 0;
									brightness = 0;
									state = idle;
								}
							}
							else
							{
								switchingon = 0x0E;
							}
						}
					}
				}
				
				if(wakefulness == 0)
				{
					state = sleeping;
				}
				
				//PORTB = (PORTB & (~(FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE))) | ((switchingon==0 || switchingon==2)? FL_RGBLED_RIGHT_ENABLE : 0) | (switchingon > 0? FL_RGBLED_LEFT_ENABLE : 0);
				
				/*
				DDRB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				PORTB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				
				if (switchingin == 1)
				{
					PORTB &= ~(FL_PWM_NOT_R);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_R);
				}
				

				if (switchingin == 2)
				{
					PORTB &= ~(FL_PWM_NOT_G);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_G);
				}
				
				
				if (switchingon > 7)
				{
					PORTB &= ~(FL_PWM_NOT_B_USBDM_MOSI);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_B_USBDM_MOSI);
				}
				
				
				if (switchingin == 0)
				{
					DDRC |= (FL_ADC5_MAINLED_VOLTAGE);
					PORTC |= (FL_ADC5_MAINLED_VOLTAGE);
				}
				else
				{
					DDRC &= ~(FL_ADC5_MAINLED_VOLTAGE);
					PORTC &= ~(FL_ADC5_MAINLED_VOLTAGE);
				}
	*/			
/*
				if ((max_button & 0b00000001) == 0b00000001)
				{
					PORTB &= ~(FL_PWM_NOT_R);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_R);
				}
				

				if ((max_button & 0b00000010) == 0b00000010)
				{
					PORTB &= ~(FL_PWM_NOT_G);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_G);
				}
				
				
				if ((max_button & 0b00000100) == 0b00000100)
				{
					PORTB &= ~(FL_PWM_NOT_B_USBDM_MOSI);
				}
				else
				{
					PORTB |= (FL_PWM_NOT_B_USBDM_MOSI);
				}
				
				
				if ((max_button & 0b00001000) == 0b00001000)
				{
					DDRC |= (FL_ADC5_MAINLED_VOLTAGE);
					PORTC |= (FL_ADC5_MAINLED_VOLTAGE);
				}
				else
				{
					DDRC &= ~(FL_ADC5_MAINLED_VOLTAGE);
					PORTC &= ~(FL_ADC5_MAINLED_VOLTAGE);
				}
				
				/*
				for(unsigned char i=0; i<4; i++)
				{
					buttons[i] = GetButtonDifference(i);
				}
				
				
				
				switch(switchingon)
				{
					case 0:
					{
						switchingin = 0;
						if ((buttons[0] < 36) && (buttons[1] < 36) && (buttons[2] < 36) && (buttons[3] < 36))
						{
							switchingon = 1; // Start detecting
							PORTB &= ~(FL_PWM_NOT_R);
						}
						break;
					}
					case 1:
					{
						unsigned char index_b = 0;
						unsigned char number_b = 0;
						for(unsigned char i=0; i<4; i++)
						{
							if (buttons[i] > 36)
							{
								index_b = i;
								number_b++;
							}
						}
						if(number_b > 1)
						{
							switchingon = 0; // Restart detecting
						}else if (number_b == 1)
						{
							switchingin = index_b;
							switchingon = 2;
							PORTB |= ~(FL_PWM_NOT_R);
							PORTB &= ~(FL_PWM_NOT_B_USBDM_MOSI);
						}
						break;
					}
					case 2:
					{
						unsigned char index_b = 0;
						unsigned char number_b = 0;
						for(unsigned char i=0; i<4; i++)
						{
							if (buttons[i] > 36)
							{
								index_b = i;
								number_b++;
							}
						}
						if(number_b > 1)
						{
							switchingon = 0; // Restart detecting
						}else if (number_b == 1)
						{
							if (switchingin == index_b) break;
							switch(switchingin)
							{
								case 0:
								{
									if(index_b == 1)
									{
										switchingin = index_b;
										switchingon = 3;
									}
									else 
									{
										switchingon = 0; // Restart detecting
									}
									break;
								}
								case 1:
								{
									if(index_b == 2)
									{
										switchingin = index_b;
										switchingon = 3;
									}
									else
									{
										switchingon = 0; // Restart detecting
									}
									break;
								}
								case 2:
								{
									if(index_b == 3)
									{
										switchingin = index_b;
										switchingon = 3;
									}
									else
									{
										switchingon = 0; // Restart detecting
									}
									break;
								}
								case 3:
								{
									if(index_b == 0)
									{
										switchingin = index_b;
										switchingon = 3;
									}
									else
									{
										switchingon = 0; // Restart detecting
									}
									break;
								}
								default: break;
							}
							switchingin = index_b;
							switchingon = 2;
						}
						break;
					}
					case 3:
					{
						state = on_one;
						break;
					}
					case 4:
					{
						break;
					}
					default: break;
				}*/
				
				/*
				unsigned char max = 0;
				unsigned char index = 0;
				for(unsigned char i=0; i<4; i++)
				{
					if (buttons[i] > max)
					{
						max = buttons[i];
						index = i;
					}
				}
				if (index_p == index || (max_p<max && max>36))
				{
					index_p = index;
					if (max > max_p)
					{
						max_p = max;
					}
					else if (max + 5 < max_p)
					{
						state = on_one;
					}
					
				}
				wakefulness--;
				
				if(wakefulness == 0)
				{
					AssignPorts();
					state = sleeping;
				}
				else
				{
					DDRB &= ~(FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
					PORTB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
					PORTB &= ~(FL_PWM_NOT_G);
				}*/
				break;
			}
			case charging:
			{
				if(GetAdcValue(6) < 0x30)
				{	// Charged
					PORTB |= FL_PWM_NOT_G | FL_PWM_NOT_R | FL_RGBLED_LEFT_ENABLE | FL_RGBLED_RIGHT_ENABLE;
					PORTB &= ~(FL_PWM_NOT_B_USBDM_MOSI);
					charge_counter = 0;
				}
				else
				{
					if(GetAdcValue(7) < 0x30)
					{ // Charging
						PORTB |= FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI | FL_RGBLED_LEFT_ENABLE | FL_RGBLED_RIGHT_ENABLE;
						PORTB &= ~(FL_PWM_NOT_R);
						charge_counter = 0;
					}
					else
					{
						PORTB |= FL_PWM_NOT_R | FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI | FL_RGBLED_LEFT_ENABLE | FL_RGBLED_RIGHT_ENABLE;
						charge_counter++;
						if(charge_counter > 250)
						{
							state = oversleep;
						}
					}
				}
				break;
			}
			case idle:
			{
//				DDRC |= (FL_ADC5_MAINLED_VOLTAGE);
//				PORTC |= (FL_ADC5_MAINLED_VOLTAGE);
				DDRB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				
				unsigned char max_button = GetButtonStatistics();
				
				if (max_button == 0)
				{
					switchingin = 0;
				}
				else if (max_button == 0x01)
				{
					if (switchingin != 1)
					{
						if(brightness > 0)
						{
							brightness--;
						}
						SetBrightness(brightness);
						switchingin = 1;
					}
				}
				else if (max_button == 0x02)
				{
					if (switchingin != 2)
					{
						if(brightness < 10)
						{
							brightness++;
						}
						SetBrightness(brightness);
						switchingin = 2;
					}
				}
				else if (max_button == 0x0F)
				{
					ResetPorts();
					state = sleeping;
				}
				
				
				//state = sleeping;
				/*state = sleeping;
				PORTB |= (FL_PWM_NOT_G);
				DDRB &= ~(FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				PORTB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
				PORTB &= ~(FL_PWM_NOT_R);*/
				break;
			}
			default:
			{
				state = sleeping;
			}
		}

		if(state != charging)
		{
			/*
			if (pwm < 100)
			{
				pwm++;
				if(Green > pwm)
				{
					DDRB &= ~(FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
					PORTB |= (FL_RGBLED_RIGHT_ENABLE | FL_RGBLED_LEFT_ENABLE);
					PORTB &= ~(FL_PWM_NOT_G);
				}
				else
				{
					PORTB |= FL_PWM_NOT_G;
				}
			}
			else
			{
				pwm = 0;
				Green--;
				for(unsigned char i=0; i<4; i++)
				{
					buttons[i] = GetButtonDifference(i);
				}
			}
			*/
		}
		wdt_reset();
	}
}