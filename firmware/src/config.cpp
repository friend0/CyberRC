#include "config.h"

#if defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32)
const int ppm_output_pins[8] = {5, 6, 9, 10, 20, 21, 22, 23};
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
const int ppm_output_pins[10] = {6, 9, 10, 11, 12, 13, 14, 15, 18, 19};
#else
    #error "Unsupported board"
#endif

PPMGenerator ppm_generator = PPMGenerator(ppm_output_pins[0], channel_values[0], MAX_NUM_CHANNELS);

uint32_t channel_values[NUM_LINES][MAX_NUM_CHANNELS];

uint8_t SERIAL_READ_BUFFER[32768];
uint8_t SERIAL_WRITE_BUFFER[4096];

/// @brief Setup the serial port for communication
void setup_serial()
{
    Serial1.begin(460800);
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

// Default control values: mid stick for control surfaces, zero throttle
static uint32_t control_defaults[4] = {1500, 1500, 1000, 1500};

/// @brief Initialize the PPM output
void initialize_ppm()
{  
    for (uint8_t i = 0; i < MAX_NUM_CHANNELS; i++)
    {
        ppm_output.updateChannel(i, control_defaults[i]);
    }
}