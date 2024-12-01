#ifndef PPMGENERATOR_H
#define PPMGENERATOR_H

#include <Arduino.h>
#include <IntervalTimer.h>

class PPMGenerator {
private:
    uint8_t pin;                         // Output pin for PPM signal
    IntervalTimer timer;                 // Timer for generating PPM
    uint32_t frameLength;                // Total frame length in microseconds
    uint32_t pulseWidth;                 // Pulse width in microseconds
    uint32_t* channelValues;             // Pointer to array of channel values
    uint8_t numChannels;                 // Number of channels
    uint8_t currentChannel = 0;          // Current channel being processed
    uint32_t remainingFrameTime;         // Remaining time in the current frame
    bool isPulse = true;                 // Toggles between pulse and gap

    // ISR handler for the timer
    void handleTimer();

    // Static wrapper for ISR callback
    static void timerCallbackWrapper();

    // Static instance pointer for accessing class members in the ISR
    static PPMGenerator* instancePtr;

public:
    // Constructor
    PPMGenerator(uint8_t outputPin, uint32_t* channels, uint8_t numCh, uint32_t frameLen = 22500, uint32_t pulseW = 300);

    // Start generating the PPM signal
    void begin();

    // Stop generating the PPM signal
    void stop();

    // Update a specific channel value
    void updateChannel(uint8_t channel, uint32_t value);
};

#endif // PPMGENERATOR_H
