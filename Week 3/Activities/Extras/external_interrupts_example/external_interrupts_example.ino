 /*
 * Copyright (c) 2019, Manchester Robotics Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Manchester Robotics Ltd. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * \file button_arduino.ino
 * \author Eduard Codres, Mario Martinez
 * \copyright Manchester Robotics Ltd.
 * \date March, 2020
 * \brief This programs uses external interrupts to read the state of a normally open (NO) pushbutton using a debounce routine.
 */

 /*
 * INSTRUCTIONS
 *  Connect a normally open (NO) pushbutton with in a pulldown resistor configuration to the ButtonPin.
    (https://www.electroduino.com/arduino-tutorial-8-arduino-digitalread-using-push-button/)
 *  ButtonPin must be selected as one with an external interrupt. Default for ESP32 23.
 *  Open the Serial Monitor
 */

/**
                  ----- +5
                    |
                    |
             Switch  \
               NO     \
                    |
                    |-----0(gpio)
                   ---
                   |r|  Resistor 10K
                   |r|
                   ---
                    |
                  __|__
                   ---   GND
                    -
*/

/*
 * ESP32 Button Debouncer using External Interrupts
 * Features:
 * - Uses attachInterrupt() with debounce handling inside ISR
 * - Toggles LED only when a full press-release cycle is detected
 * - Uses a non-blocking loop for better performance
 */

#define LedPin  22  // Built-in LED on ESP32 (adjust if needed)
#define ButtonPin  23  // Button connected to GPIO 3 (adjust if needed)
#define DEBOUNCE_DELAY 50  // Debounce time in milliseconds (ESP32 needs higher debounce time)

// Button handling variables
volatile bool interruptStatus = false; // Tracks ISR trigger state
volatile unsigned long lastInterruptTime = 0;  // Tracks last button press time
volatile long counter = 0;  // // Button press counter
bool ledStatus = LOW;  // LED state
int32_t counter = 0;   // Button press counter

// ISR (Interrupt Service Routine) - Handles button press detection
void IRAM_ATTR interruptHandler() {
  unsigned long currentTime = millis();
  if ((currentTime - lastInterruptTime) > DEBOUNCE_DELAY) {  
    interruptStatus = true;
    lastInterruptTime = currentTime;  // Update last debounce time
  }
}

// Debounced Button Read Function
bool buttonRead() {
  static bool buttonPressed = false;
  bool buttonState = digitalRead(ButtonPin);  // Read button state

  if (interruptStatus) {  
    if (buttonState) {  // Button is pressed
      buttonPressed = true;
    } 

    if (buttonPressed && !buttonState) {  // Button was pressed and now released
      buttonPressed = false;  // Reset state
      interruptStatus = false;  // Allow ISR to trigger again
      return true;  // Confirm valid press-release cycle
    }
  }
  return false;  // No valid button press detected
}

void setup() {
  Serial.begin(115200);  // Start Serial Monitor for debugging
  pinMode(LedPin, OUTPUT);
  pinMode(ButtonPin, INPUT);  // Use internal pull-up resistor

  attachInterrupt(digitalPinToInterrupt(ButtonPin), interruptHandler, CHANGE); // Detect both press and release
}

void loop() {
  if (buttonRead()) {
    ledStatus = !ledStatus;  // Toggle LED state
    digitalWrite(LedPin, ledStatus);
    counter++;  // Increment counter
    Serial.print("Button Pressed! Count: ");
    Serial.println(counter);
  }
}