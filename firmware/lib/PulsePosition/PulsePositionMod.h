/* PulsePosition Library for Teensy 3.x, LC, and 4.0

 * High resolution input and output of PPM encoded signals
 * http://www.pjrc.com/teensy/td_libs_PulsePosition.html
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this library was funded by PJRC.COM, LLC by sales of Teensy
 * boards.  Please support PJRC's efforts to develop open source software by
 * purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>

#ifdef __AVR__
#error "Sorry, PulsePosition does not work on Teensy 2.0 and other AVR-based boards"
#elif defined(__IMXRT1062__)
#include "PulsePositionIMXRT.h"
#else 

#define PULSEPOSITION_MAXCHANNELS 16

// The shortest time allowed between any 2 rising edges.  This should be at
// least double TX_PULSE_WIDTH.
#ifndef TX_MINIMUM_SIGNAL
#define TX_MINIMUM_SIGNAL   300.0
#endif

// The longest time allowed between any 2 rising edges for a normal signal.
#ifndef TX_MAXIMUM_SIGNAL
#define TX_MAXIMUM_SIGNAL  2500.0
#endif

// The default signal to send if nothing has been written.
#ifndef TX_DEFAULT_SIGNAL
#define TX_DEFAULT_SIGNAL  1500.0
#endif

// When transmitting with a single pin, the minimum space signal that marks
// the end of a frame.  Single wire receivers recognize the end of a frame
// by looking for a gap longer than the maximum data size.  When viewing the
// waveform on an oscilloscope, set the trigger "holdoff" time to slightly
// less than TX_MINIMUM_SPACE, for the most reliable display.  This parameter
// is not used when transmitting with 2 pins.
#ifndef TX_MINIMUM_SPACE
#define TX_MINIMUM_SPACE   5000.0
#endif

// The minimum total frame size.  Some servo motors or other devices may not
// work with pulses the repeat more often than 50 Hz.  To allow transmission
// as fast as possible, set this to the same as TX_MINIMUM_SIGNAL.
#ifndef TX_MINIMUM_FRAME
#define TX_MINIMUM_FRAME  20000.0
#endif

// The length of all transmitted pulses.  This must be longer than the worst
// case interrupt latency, which depends on how long any other library may
// disable interrupts.  This must also be no more than half TX_MINIMUM_SIGNAL.
// Most libraries disable interrupts for no more than a few microseconds.
// The OneWire library is a notable exception, so this may need to be lengthened
// if a library that imposes unusual interrupt latency is in use.
#ifndef TX_PULSE_WIDTH
#define TX_PULSE_WIDTH      100.0
#endif

// When receiving, any time between rising edges longer than this will be
// treated as the end-of-frame marker.
#ifndef RX_MINIMUM_SPACE
#define RX_MINIMUM_SPACE   3500.0
#endif

typedef struct {
    float tx_minimum_signal;
    float tx_maximum_signal;
    float tx_default_signal;
    float tx_minimum_space;
    float tx_minimum_frame;
    float tx_pulse_width;
    float rx_minimum_space;
} PPMConfig;

extern PPMConfig ppm_config;

struct ftm_channel_struct {
	uint32_t csc;
	uint32_t cv;
};

class PulsePositionOutput
{
public:
	PulsePositionOutput(void);
	PulsePositionOutput(int polarity);
	bool begin(uint8_t txPin); // txPin can be 5,6,9,10,20,21,22,23
	bool begin(uint8_t txPin, uint8_t framePin);
	bool write(uint8_t channel, float microseconds);
	friend void ftm0_isr(void);
private:
	void isr(void);
	uint32_t pulse_width[PULSEPOSITION_MAXCHANNELS+1];
	uint32_t pulse_buffer[PULSEPOSITION_MAXCHANNELS+1];
	uint32_t pulse_remaining;
	volatile uint8_t *framePinReg;
	volatile uint8_t framePinMask;
	struct ftm_channel_struct *ftm;
	uint8_t state;
	uint8_t current_channel;
	uint8_t total_channels;
	uint8_t total_channels_buffer;
	uint8_t cscSet;
	uint8_t cscClear;
	static PulsePositionOutput *list[8];
	static uint8_t channelmask;
};


class PulsePositionInput
{
public:
	PulsePositionInput(void);
	PulsePositionInput(int polarity);
	bool begin(uint8_t rxPin); // rxPin can be 5,6,9,10,20,21,22,23
	int available(void);
	float read(uint8_t channel);
	friend void ftm0_isr(void);
private:
	void isr(void);
	struct ftm_channel_struct *ftm;
	uint32_t pulse_width[PULSEPOSITION_MAXCHANNELS];
	uint32_t pulse_buffer[PULSEPOSITION_MAXCHANNELS];
	uint32_t prev;
	uint8_t write_index;
	uint8_t total_channels;
	uint8_t cscEdge;
	bool available_flag;
	static bool overflow_inc;
	static uint16_t overflow_count;
	static PulsePositionInput *list[8];
	static uint8_t channelmask;
};

#endif
