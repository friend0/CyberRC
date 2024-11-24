#include "config.h"

#if defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32)
const int ppm_output_pins[8] = {5, 6, 9, 10, 20, 21, 22, 23};
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
const int ppm_output_pins[10] = {6, 9, 10, 11, 12, 13, 14, 15, 18, 19};
#else
    #error "Unsupported board"
#endif

PulsePositionOutput output_channels[NUM_LINES];

uint32_t channel_values[MAX_NUM_CHANNELS];

void setup_ppm()
{
    for (int i=0; i<NUM_LINES; i++)
    {
      output_channels[i] = PulsePositionOutput(FALLING);
      output_channels[i].begin(ppm_output_pins[i]);
      for (int j=0; j<MAX_NUM_CHANNELS; j++)
      {
        output_channels[i].write(j, 1500);
      }
    }
    for (int i=0; i<MAX_NUM_CHANNELS; i++)
    {
      channel_values[i] = 1500;
    }
}
