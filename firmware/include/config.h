#include <Arduino.h>
#include "RCData.pb.h"
#include "ppm.h"

#ifndef CONFIG_H
#define CONFIG_H

// Config
#define DEBUG
// provide the number of PPMs to be used
// as of 7/18/24 we are only using 1 PPM
#define NUM_LINES 1

// todo: PPM library does not make this configurable out of the box
// you will need to update the #define in PulsePosition.h until this is implemented as a new feature
#define NUM_PPM_CHANNELS 4 // set the number of channels per line

#if defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32)
extern const uint8_t ppm_output_pins[8];
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
extern const uint8_t ppm_output_pins[10];
#else
    #error "Unsupported board"
#endif

extern PPMGenerator<NUM_PPM_CHANNELS> ppm_output;
extern u_int32_t channel_values[NUM_PPM_CHANNELS];

void setup_serial();

void initialize_ppm();

#endif // CONFIG_H
