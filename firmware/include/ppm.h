/* Frame Length: 10ms
Inverted PPM Signal Explained:

In this configuration, the pulse (300µs) is low, and the gaps (high) represent channel values.
A long low sync pulse at the end marks the frame boundary.

Pulse Length: 300µs
4 Channels: High gaps represent the channel positions
-----------------------------------------------

PPM Signal (Time in milliseconds) Each pulse between channels is 300µs.
The length of time falling edges is the channel value (rising edges if the PPM is not inverted).
Each channel is limited to the range 1000µs to 2000µs.
The sync time will fill the remaining time to make each frame 10ms.

* Note * The example waveform below is a FALLING edge, or negative polarity PPM signal.
A RISING, or positive polarity PPM signal would have the 300us pulses as positive, and the channel delays as negative.

  <--------------------------------------------------10ms --------------------------------------------------->
       ______________     __________     ______________     _________________     _____________________________
  X___|              |___|          |___|              |___|                 |___|                             |___X
       <------1.7ms------><----1.3ms----><------1.8ms------><--------2.0ms-------><------------ Sync --------->  

 */

#ifndef PPMGENERATOR_H
#define PPMGENERATOR_H


#include <Arduino.h>
#include <IntervalTimer.h>

template <size_t N>
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
    uint8_t polarity;                    // PPM signal polarity    
    bool isPulse = true;                 // Toggles between pulse and gap

    // ISR handler for the timer
    void handleTimer() {
        if (isPulse) {
            // Generate a pulse
            if (polarity == RISING) {
                digitalWriteFast(pin, LOW);
            } else {
                digitalWriteFast(pin, HIGH);
            }
            timer.update(pulseWidth); // Set the timer for the pulse width
            isPulse = false;
            // Deduct the pulse width from the remaining frame time
            remainingFrameTime -= pulseWidth;
        } else {
            // Modulate pulse position 
            if (polarity == RISING) {
                digitalWriteFast(pin, HIGH);
            } else {
                digitalWriteFast(pin, LOW);
            }
            if (currentChannel < numChannels) {
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
    };

    // Static wrapper for ISR callback
    static void timerCallbackWrapper() {
        if (instancePtr) {
            instancePtr->handleTimer();
        }
    };

    // Static instance pointer for accessing class members in the ISR
    static PPMGenerator* instancePtr;

public:
    // TODO: make constructor paramter order match the parameter lits/private variable defintion order
    PPMGenerator(const uint8_t pin, uint32_t (&channels)[N], uint8_t numChannels, uint32_t frameLength = 10000, uint32_t pulseWidth = 300, int polarity = FALLING) 
    : pin(pin), frameLength(frameLength), pulseWidth(pulseWidth), channelValues(channels), numChannels(numChannels), polarity(polarity) {
        remainingFrameTime = frameLength;
        instancePtr = this; // Set the static instance pointer
    };

    // Start generating the PPM signal
    void begin() {
        pinMode(pin, OUTPUT);
        if (polarity == RISING) {
            digitalWrite(pin, HIGH); // Start with the PPM pin high 
        } else {
            digitalWrite(pin, LOW); // Start with the PPM pin low
        }
        timer.begin(timerCallbackWrapper, 1500); // Start the timer
    };

    // Stop generating the PPM signal
    void stop() {
        if (polarity == RISING) {
            digitalWrite(pin, HIGH); // Stop with the pin high 
        } else {
            digitalWrite(pin, LOW); // Stop with the pin low
        }
        digitalWrite(pin, HIGH);
    };

    // Update a specific channel value
    void updateChannel(uint8_t channel, uint32_t value) {
        if (channel < numChannels) {
            channelValues[channel] = value;
        }
    };

    void updateChannels(uint32_t* values) {
        channelValues = values;
    };
};

// Static instance pointer
template <size_t N>
PPMGenerator<N>* PPMGenerator<N>::instancePtr = nullptr;

#endif // PPMGENERATOR_H
