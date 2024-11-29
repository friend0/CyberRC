#include "config.h"

#if defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32)
const int ppm_output_pins[8] = {5, 6, 9, 10, 20, 21, 22, 23};
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
const int ppm_output_pins[10] = {6, 9, 10, 11, 12, 13, 14, 15, 18, 19};
#else
    #error "Unsupported board"
#endif

PulsePositionOutput output_channels[NUM_LINES];
// PulsePositionOutput ppm_output;
uint32_t channel_values[NUM_LINES][MAX_NUM_CHANNELS];

uint8_t SERIAL_READ_BUFFER[32768];
uint8_t SERIAL_WRITE_BUFFER[4096];

void setup_serial()
{
    Serial1.begin(230400);
    Serial1.addMemoryForRead(&SERIAL_READ_BUFFER, sizeof(SERIAL_READ_BUFFER));
    Serial1.addMemoryForWrite(&SERIAL_WRITE_BUFFER, sizeof(SERIAL_WRITE_BUFFER));
    Serial1.flush();
    while (!Serial1 && millis() < 5000)
    {
        delay(100);
    }
    if (CrashReport)
    {
        Serial1.print(CrashReport);
        Serial1.println();
        Serial1.flush();
    }
}

void setup_ppm()
{
    Serial1.println("Setting up PPM");
    for (int i=0; i<NUM_LINES; i++)
    {
      Serial1.printf("Setting up channel %d\r\n", i);
      Serial1.printf("Initializing PPM %d on pin %d\r\n", i, ppm_output_pins[i]);
      Serial1.flush();
      output_channels[i] = PulsePositionOutput(FALLING);
      output_channels[i].begin(ppm_output_pins[i]);
      for (int j = 1; j <= MAX_NUM_CHANNELS; j++)
      {
        output_channels[i].write(j, 1500);
      }
      for (int j=0; j<MAX_NUM_CHANNELS; j++)
      {
        output_channels[i].write(j + 1, 1500);
      }
    }
}

