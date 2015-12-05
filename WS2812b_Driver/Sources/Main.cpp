/*
 * Main.cpp
 *
 *  Created on: 31 ���. 2015 �.
 *      Author: Admin
 */

//#define WS2812B_HSV_DEMO
//#define READ_MANUAL


#ifndef WS2812B_HSV_DEMO

#include <Arduino.h>
#include <stdint.h>

#include "ws2812b_write.h"
#include "ws2812b_types.h"


static ws2812b::color_rgb frameBuffer[60] = {};


void fillColor(ws2812b::color_rgb *frameBuffer, size_t count, const ws2812b::color_rgb &color);

template<size_t count>
void fillColor(ws2812b::color_rgb (&frameBuffer)[count], const ws2812b::color_rgb &color) {
	fillColor(frameBuffer, count, color);
}


static bool stateChange;

void loop() {
	if (!!Serial.available()) {
#ifdef READ_MANUAL
		byte *frame = reinterpret_cast<byte *>(frameBuffer);

		byte countRead = 0;
		while (countRead < sizeof(frameBuffer)) {
			int value = Serial.read();

			if (value >= 0x00) {
				*frame++ = static_cast<byte>(value);

				++countRead;
			}
		}
#else
		Serial.readBytes(reinterpret_cast<byte*>(frameBuffer), sizeof(frameBuffer));
#endif // MANUAL_READ

		PORTD |=  _BV(PORTD2); // Disable CTS.
		{
			ws2812b::write<13>(frameBuffer);

			stateChange = true;
		}
		PORTD &= ~_BV(PORTD2); // Enable CTS.
	} else {
		if (stateChange) {
			stateChange = false;

			memset(frameBuffer, 0x00, sizeof(frameBuffer));
			ws2812b::write<13>(frameBuffer);
		}
	}
}

void setup() {
	Serial.begin(62500);

	DDRB |= _BV(DDB5); // Pin 13 - display output.
	DDRD |= _BV(DDD2); // Pin 2  - CTS signal output.

	PORTB &= ~_BV(PORTB5);
	PORTD &= ~_BV(PORTD2); // Enable CTS.

	ws2812b::color_rgb color;

	color.r = 0xff;
	color.g = 0x00;
	color.b = 0x00;
	fillColor(frameBuffer, color);
	ws2812b::write<13>(frameBuffer);
	delay(500);

	color.r = 0x00;
	color.g = 0xff;
	color.b = 0x00;
	fillColor(frameBuffer, color);
	ws2812b::write<13>(frameBuffer);
	delay(500);

	color.r = 0x00;
	color.g = 0x00;
	color.b = 0xff;
	fillColor(frameBuffer, color);
	ws2812b::write<13>(frameBuffer);
	delay(500);

	memset(reinterpret_cast<byte*>(&frameBuffer), 0x00, sizeof(frameBuffer));
	ws2812b::write<13>(frameBuffer);
}


void fillColor(ws2812b::color_rgb *frameBuffer, size_t count, const ws2812b::color_rgb &color) {
	while(count > 0) {
		frameBuffer[--count] = color;
	}
}

#else  // ifdef WS2812B_HSV_DEMO

#include <avr/pgmspace.h>
#include <Arduino.h>
#include <stdint.h>

#include "ws2812b_Screen.h"


static ws2812b::Screen<2> screenMain;

const byte dim_curve[] PROGMEM = {
    0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
    8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
    11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
    15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
    20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
    27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
    36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
    48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
    63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
    83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
    110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
    146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
    193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};

void hsv2rgb(int hue, uint8_t sat, uint8_t val, ws2812b::color_rgb &output) {
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.
  */
  val = pgm_read_byte(dim_curve + val);
  sat = 255 - pgm_read_byte(dim_curve + 255 - sat);

  if (sat == 0) { // Achromatic color (gray). Hue doesn't mind.
    output.r=val;
    output.g=val;
    output.b=val;
  } else  {
    hue = hue % 360;

    if (hue < 0) {
    	hue += 360;
    }
    int base = ((255 - sat) * val)>>8;

    switch((hue/60) % 6) {
	case 0:
		output.r = val;
		output.g = (((val-base)*hue)/60)+base;
		output.b = base;
		break;

	case 1:
		output.r = (((val-base)*(60-(hue%60)))/60)+base;
		output.g = val;
		output.b = base;
		break;

	case 2:
		output.r = base;
		output.g = val;
		output.b = (((val-base)*(hue%60))/60)+base;
		break;

	case 3:
		output.r = base;
		output.g = (((val-base)*(60-(hue%60)))/60)+base;
		output.b = val;
		break;

	case 4:
		output.r = (((val-base)*(hue%60))/60)+base;
		output.g = base;
		output.b = val;
		break;

	case 5:
		output.r = val;
		output.g = base;
		output.b = (((val-base)*(60-(hue%60)))/60)+base;
		break;
    }
  }
}


static int  readings[16] = {};
static long totalReadings;

static uint8_t  inputIndex;

static bool hasNextValue(int &value) {
	totalReadings -= readings[inputIndex];
	{
		readings[inputIndex] = analogRead(A0);
	}
	totalReadings += readings[inputIndex];

	if ((inputIndex = (inputIndex + 1) % _countof(readings)) == 0) {
		value = totalReadings / _countof(readings);

		return true;
	}
	return false;
}


static int		hue 		=  0;
static uint8_t 	saturation 	=  0;
static uint8_t 	value 		=  255;


static void toggleTimer1() {
	cli();
	{
		TCCR1B = (TCCR1B & ~_BV(CS12)) | ((TCCR1B & _BV(CS12)) ^ _BV(CS12));
	}
	sei();
}

static void disableTimer1() {
	cli();
	{
		TCCR1B = TCCR1B & ~_BV(CS12);
	}
	sei();
}


static volatile bool changed = true;

ISR(TIMER1_COMPA_vect) {
	hue = (hue + 1) % 1024;

	changed = true;
}


static int inputValuePrevious;

void loop() {
	int inputValue;
	if (hasNextValue(inputValue)
			&& abs(static_cast<uint16_t>(inputValue) - inputValuePrevious) > 1) {

		inputValuePrevious = inputValue;
		hue = inputValue;

		disableTimer1();
		changed = true;
	}
	if (Serial.available()) {
		int cmd = Serial.read();

		switch (cmd) {
		case '+':
			saturation = std::min(saturation + 12, 255);

			changed = true;
			break;

		case '-':
			saturation = std::max(saturation - 12, 0);

			changed = true;
			break;

		case 'z':
			toggleTimer1();
			break;
		}
	}
	if (changed) {
		changed = false;

		ws2812b::color_rgb color;
		hsv2rgb(map(hue, 0, 1024, 0, 360), saturation, value, color);

		std::fill(screenMain.getLine(0), screenMain.getLine(0) + ws2812b::Screen<2>::SCREEN_AREA, color);
		screenMain.display();
	}
}

void setup() {
	DDRB |= B00100000;
	DDRD |= B00000100;

	Serial.begin(115200);
	screenMain.init();

	cli();
	{
		TCCR1A = 0;
		TCCR1B = 0;

		OCR1A  = 819;							// ~70Hz
		TCCR1B = _BV(WGM12) /*| _BV(CS12)*/;	// CTC mode, 256 pre-scaler

		TCNT1  = 0;
		TIMSK1 = _BV(OCIE1A);
	}
	sei();

	PORTB = B00100000;
}

#endif // ifndef WS2812B_HSV_DEMO