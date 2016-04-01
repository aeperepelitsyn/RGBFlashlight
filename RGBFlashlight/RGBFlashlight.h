////////////////////////////////////////////////////////////////////////////////
// File       : RGBFlashlight.h
// Description: Main firmware header for prototype of RGB Flashlight (20150630)
// Created    : 2015.07.05 16:07:32
// Author     : aeperepelitsyn
// Version    : 1.00 (2015.09.07 21:15:42)
////////////////////////////////////////////////////////////////////////////////

// PORTB
#define	FL_RGBLED_RIGHT_ENABLE 	0b10000000
#define	FL_RGBLED_LEFT_ENABLE	0b01000000
#define	FL_USBNC_SCK 			0b00100000
#define	FL_USBDP_MISO 			0b00010000
#define	FL_PWM_NOT_B_USBDM_MOSI 0b00001000
#define	FL_PWM_NOT_G			0b00000100
#define	FL_PWM_NOT_R			0b00000010
#define	FL_RGBLED_MAIN_DISABLE 	0b00000001

// PORTC
#define	FL_ADC5_MAINLED_VOLTAGE 0b00100000
#define	FL_ADC_PULLUP_ENABLE	0b00010000
#define	FL_OPTRON_3				0b00001000
#define	FL_OPTRON_2				0b00000100
#define	FL_OPTRON_1				0b00000010
#define	FL_OPTRON_0				0b00000001
#define	FL_OPTRONS				0b00001111

// PORTD
#define	FL_MAINLED_DCDC_ENABLE	0b00010000

unsigned char statistics[16] = {};

inline void ResetPorts()
{
	PORTB = FL_RGBLED_MAIN_DISABLE | FL_PWM_NOT_R | FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI;
	DDRB  = //FL_RGBLED_LEFT_ENABLE | FL_RGBLED_RIGHT_ENABLE |
			FL_RGBLED_MAIN_DISABLE | FL_PWM_NOT_R | FL_PWM_NOT_G | FL_PWM_NOT_B_USBDM_MOSI;
	
	PORTC = 0;
	DDRC = 0;
	
	PORTD = 0;
	DDRD = FL_MAINLED_DCDC_ENABLE | FL_OPTRONS;
}


ISR(ADC_vect)
{
	ADCSRA ^= 0b10000000; // Disable the ADC.
	cli();
}

unsigned char GetAdcValue(unsigned char adcChannel)
{	// ADC0 to ADC7 (0x00 to 0x07), 1.30V VBG (0x0E), 0V GND (0x0F).
	ADMUX = (0b01100000 | (0b00001111 & adcChannel));
	ADCSRA = 0b10001111;
	ADCSRA |= 0x40;
	sei();
	set_sleep_mode(SLEEP_MODE_ADC);
	sleep_mode();
	return ADCH;
}

unsigned char GetButtonDifference (unsigned char buttonNumber)
{
	unsigned char withLight, withoutLight, difference;
	PORTC |= FL_ADC_PULLUP_ENABLE;
	DDRC  |= FL_ADC_PULLUP_ENABLE;
	
#ifdef FL_STUB_2015
	if(buttonNumber == 2)
	{
		DDRC |= (FL_OPTRONS^FL_OPTRON_2);
		DDRC &=~FL_ADC_PULLUP_ENABLE;
		PORTC |= (FL_OPTRONS^FL_OPTRON_2);
		PORTC &= ~FL_ADC_PULLUP_ENABLE;
	} else if(buttonNumber == 3)
	{
		DDRC |= (FL_OPTRONS^FL_OPTRON_3);
		PORTC |= (FL_OPTRONS^FL_OPTRON_3);
		DDRC &=~FL_ADC_PULLUP_ENABLE;
		PORTC &= ~FL_ADC_PULLUP_ENABLE;
	}
#endif

	PORTD |= (1<<buttonNumber);
	withLight = GetAdcValue(buttonNumber);
	
#ifdef FL_STUB_2015
	if(buttonNumber == 2 || buttonNumber == 3)
	{
		withoutLight = GetAdcValue(4);
	}
#endif

	PORTD ^= (1<<buttonNumber);
	withoutLight = GetAdcValue(buttonNumber);
	
#ifdef FL_STUB_2015
	if(buttonNumber == 2 || buttonNumber == 3)
	{
		withoutLight = GetAdcValue(4);
	}
	if(buttonNumber == 2)
	{
		DDRC |=FL_ADC_PULLUP_ENABLE;
		DDRC &= ~(FL_OPTRONS^FL_OPTRON_2);
		PORTC &= ~(FL_OPTRONS^FL_OPTRON_2);
		PORTC |= FL_ADC_PULLUP_ENABLE;
	}
	else if(buttonNumber == 3)
	{
		DDRC |=FL_ADC_PULLUP_ENABLE;
		DDRC &= ~(FL_OPTRONS^FL_OPTRON_3);
		PORTC |= FL_ADC_PULLUP_ENABLE;
		PORTC &= ~(FL_OPTRONS^FL_OPTRON_3);
	}
#endif
	
	if (withLight < withoutLight)
	{
		difference = withoutLight - withLight;
		
#ifdef FL_STUB_2015
		if(buttonNumber == 2 || buttonNumber == 3)
		{
			difference = difference << 2;
		}
		else if(buttonNumber == 3)
		{
			difference = difference << 1;
		}
#endif

	}
	else
	{
		difference = 0;
	}
	
	DDRC  ^= FL_ADC_PULLUP_ENABLE;
	PORTC ^= FL_ADC_PULLUP_ENABLE;
	return difference;
}

unsigned char button_levels[4] = {};

unsigned char GetButtons()
{
	unsigned char buttons = 0;
	for(unsigned char i=0; i<4; i++)
	{
		buttons |= ((GetButtonDifference(i) > (button_levels[i]-(button_levels[i]>>2)) ? 1 : 0)<<i);
	}
	return buttons;
}

unsigned char GetButtonStatistics()
{
	unsigned char buttons = GetButtons();
	unsigned char max_statistic = 0;
	unsigned char max_button = 0;
	for(unsigned char i=0; i<sizeof(statistics); i++)
	{
		if(i == buttons)
		{
			if(statistics[i] < 10)
			{
				statistics[i]++;
			}
		}
		else
		{
			for (unsigned char z=0; z<(i>0?1:2); z++)
			{
				if(statistics[i]>0)
				{
					statistics[i]--;
				}
			}
		}
		if(statistics[i] > max_statistic)
		{
			max_statistic = statistics[i];
			max_button = i;
		}
	}
	return max_button;
}

/*
void PinsTest()
{
	DDRB |= 0b11001111;
	PORTB ^= 0b11000001;
	PORTB |= 0b00001110;
	PORTB ^= 0b00000010;
	//PORTD ^= 0b10000000;
	//PORTD ^= 0b01000000;
	//PORTD ^= 0b00100000;
	PORTC ^= 0b00000000;
	
	DDRD =  0b00111111;
	
	_delay_ms(80);
	PORTB ^= 0b00000110;
	_delay_ms(80);
	PORTB ^= 0b00001100;
	_delay_ms(80);
	PORTB ^= 0b00001010;
	
	//PORTD ^= 0b00100000;
	DDRD =  0b01011111;
	
	_delay_ms(80);
	PORTB ^= 0b00000110;
	_delay_ms(80);
	PORTB ^= 0b00001100;
	_delay_ms(80);
	PORTB ^= 0b00001010;
	
	//PORTD ^= 0b10000000;
	//PORTD ^= 0b00100000;

	DDRD =  0b10011111;

	_delay_ms(80);
	PORTB ^= 0b00000110;
	_delay_ms(80);
	PORTB ^= 0b00001100;
	_delay_ms(80);
	PORTB ^= 0b00001010;
	
	//PORTD ^= 0b00100000;
	
	DDRD =  0b00011111;
	
	_delay_ms(80);
	PORTB ^= 0b00000110;
	_delay_ms(80);
	PORTB ^= 0b00001100;
	_delay_ms(80);
	PORTB ^= 0b00001000;
}



#define RED OCR1A
#define GREEN OCR1B
#define BLUE OCR2
#define TICKS_COUNT 250

inline void init_pwm()
{
	TCCR2 = 0b01101001;
	TCCR1A = 0b10100001;
	TCCR1B = 0b00001010;
	TIMSK |= 0b00000100; // Enable TIMER1_OVF_vect
	RED = ~0; // RED
	GREEN = ~0; // GREEN
	BLUE = ~0; // BLUE
}

struct Color
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

struct Timings
{
	unsigned short red;
	unsigned short green;
	unsigned short blue;
};

short b_diff = 0;
short r_diff = 0;
short g_diff = 0;

Color new_color;
Color old_color;
Timings color_timings;
Timings color_counters;

unsigned short color_counter = 0;
unsigned char pwm_counter = 0;
unsigned char direction = 0; // 0 increase, 1 decrease


inline unsigned char get_random_value()
{
	return rand() % 80;
}

void get_random_color()
{
	srand(117);
	new_color.blue = ~get_random_value();
	new_color.green = ~get_random_value();
	new_color.red = ~get_random_value();
}

SIGNAL(TIMER1_OVF_vect)
{
	if(b_diff == 0 && r_diff == 0 && g_diff == 0)
	{
		get_random_color();
	}
	
	r_diff = RED - new_color.red;
	if(r_diff != 0)
	{
		if(pwm_counter <= TICKS_COUNT)
		{
			pwm_counter++;
		}
		else
		{
			if(r_diff > 0) // Dencrease
			{
				RED--;
			}
			else if(r_diff < 0) // Increase
			{
				RED++;
			}
			pwm_counter = 0;
		}
	}
	
	g_diff = GREEN - new_color.green;
	if(g_diff != 0)
	{
		if(pwm_counter <= TICKS_COUNT)
		{
			pwm_counter++;
		}
		else
		{
			if(g_diff > 0) // Dencrease
			{
				GREEN--;
			}
			else if(g_diff < 0) // Increase
			{
				GREEN++;
			}
			pwm_counter = 0;
		}
	}
	
	b_diff = BLUE - new_color.blue;
	if(b_diff != 0)
	{
		if(pwm_counter <= TICKS_COUNT)
		{
			pwm_counter++;
		}
		else
		{
			if(b_diff > 0) // Dencrease
			{
				BLUE--;
			}
			else if(b_diff < 0) // Increase
			{
				BLUE++;
			}
			pwm_counter = 0;
		}
	}
}
*/