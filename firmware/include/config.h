#include <Arduino.h>
#include "PulsePositionMod.h"
#include "RCData.pb.h"

#ifndef CONFIG_H
#define CONFIG_H

// Config
#define DEBUG
// provide the number of PPMs to be used
// as of 7/18/24 we are only using 1 PPM
#define NUM_LINES 1

// todo: PPM library does not make this configurable out of the box
// you will need to update the #define in PulsePosition.h until this is implemented as a new feature
#define MAX_NUM_CHANNELS 16 // set the number of channels per line

#if defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32)
extern const int ppm_output_pins[8];
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
extern const int ppm_output_pins[10];
#else
    #error "Unsupported board"
#endif

extern PulsePositionOutput output_channels[NUM_LINES];

extern u_int32_t channel_values[NUM_LINES][MAX_NUM_CHANNELS];

void setup_serial();

void setup_ppm();

#endif // CONFIG_H
