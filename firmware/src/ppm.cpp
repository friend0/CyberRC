#include "ppm.h"

// Static instance pointer
PPMGenerator* PPMGenerator::instancePtr = nullptr;

// Constructor
PPMGenerator::PPMGenerator(uint8_t outputPin, uint32_t* channels, uint8_t numCh, uint32_t frameLen, uint32_t pulseW)
    : pin(outputPin), channelValues(channels), numChannels(numCh), frameLength(frameLen), pulseWidth(pulseW) {
    remainingFrameTime = frameLength;
    instancePtr = this; // Set the static instance pointer
}

// Start generating the PPM signal
void PPMGenerator::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // Start with the PPM pin low
    timer.begin(timerCallbackWrapper, 1500); // Start the timer
}

// Stop generating the PPM signal
void PPMGenerator::stop() {
    timer.end();
    digitalWrite(pin, LOW); // Ensure the pin is set low
}

// Update a specific channel value
void PPMGenerator::updateChannel(uint8_t channel, uint32_t value) {
    if (channel < numChannels) {
        channelValues[channel] = value;
    }
}

// ISR handler for the timer
void PPMGenerator::handleTimer() {
    if (isPulse) {
        // Generate a pulse
        digitalWrite(pin, LOW);
        timer.update(pulseWidth); // Set the timer for the pulse width
        isPulse = false;

        // Deduct the pulse width from the remaining frame time
        remainingFrameTime -= pulseWidth;
    } else {
        // Generate a gap
        digitalWrite(pin, HIGH);

        if (currentChannel < numChannels) {
            // Set timer for the channel value
            timer.update(channelValues[currentChannel] - pulseWidth);
            remainingFrameTime -= (channelValues[currentChannel] - pulseWidth);
            currentChannel++;
        } else {
            // End of frame, send the sync gap
            timer.update(remainingFrameTime);
            currentChannel = 0;         // Reset channel counter
            remainingFrameTime = frameLength;  // Reset frame time
        }

        isPulse = true;
    }
}

// Static wrapper for the ISR
void PPMGenerator::timerCallbackWrapper() {
    if (instancePtr) {
        instancePtr->handleTimer();
    }
}