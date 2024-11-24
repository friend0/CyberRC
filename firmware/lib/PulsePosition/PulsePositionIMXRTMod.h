/* PulsePosition Library for Teensy 3.1
 * High resolution input and output of PPM encoded signals
 * http://www.pjrc.com/teensy/td_libs_PulsePosition.html
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
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
 * notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __PULSE_POSITION_IMXRT_H__
#define __PULSE_POSITION_IMXRT_H__

#if defined(__IMXRT1062__)

#include <Arduino.h>

#define PULSEPOSITION_MAXCHANNELS 16

// Timing parameters, in microseconds.


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

class PulsePositionBase {
public:
protected:
  static PulsePositionBase *list[10];
  virtual void isr() = 0;

  typedef struct {
    uint8_t pin;
    uint8_t channel;
    volatile IMXRT_TMR_t *tmr;
    volatile uint32_t *clock_gate_register;
    uint32_t clock_gate_mask;
    IRQ_NUMBER_t interrupt;
    void (*isr)();
    volatile uint32_t
        *select_input_register; // Which register controls the selection
    const uint32_t select_val;  // Value for that selection
  } TMR_Hardware_t;

  static const TMR_Hardware_t hardware[];
  static const uint8_t _hardware_count;

  // static class functions

  static void isrTimer1();
  static void isrTimer2();
  static void isrTimer3();
  static void isrTimer4();
  static inline void
  checkAndProcessTimerCHInPending(uint8_t index,
                                  volatile IMXRT_TMR_CH_t *tmr_ch);
};

class PulsePositionOutput : public PulsePositionBase {
public:
  PulsePositionOutput(void);
  PulsePositionOutput(int polarity);
  bool begin(uint8_t txPin); // txPin can be 6,9,10,11,12,13,14,15,18,19
  bool begin(uint8_t txPin, uint32_t _framePin);
  bool write(uint8_t channel, float microseconds);

private:
  uint8_t outPolarity = 1; // Polarity rising
  uint8_t inPolarity = 1;

  volatile uint8_t framePinMask;
  uint32_t state, total_channels, total_channels_buffer, pulse_remaining,
      current_channel, framePin = 255;
  volatile uint32_t ticks;

  uint32_t pulse_width[PULSEPOSITION_MAXCHANNELS + 1];
  uint32_t pulse_buffer[PULSEPOSITION_MAXCHANNELS + 1];

  // member variables...
  uint16_t idx_channel;
  virtual void isr();
};

class PulsePositionInput : public PulsePositionBase {
public:
  PulsePositionInput(void);
  PulsePositionInput(int polarity);
  bool begin(uint8_t rxPin); // rxPin can be 6,9,10,11,12,13,14,15,18,19
  int available(void);
  float read(uint8_t channel);

private:
  uint32_t pulse_width[PULSEPOSITION_MAXCHANNELS + 1];
  uint32_t pulse_buffer[PULSEPOSITION_MAXCHANNELS + 1];
  uint32_t prev;
  uint8_t write_index;
  uint8_t total_channels;
  uint8_t outPolarity = 1; // Polarity rising
  uint8_t inPolarity = 1;
  volatile uint32_t ticks, overflow_count;
  volatile bool overflow_inc, available_flag;
  static uint8_t channelmask;
  // member variables...
  uint16_t idx_channel;
  virtual void isr();
};

#endif
#endif
