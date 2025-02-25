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
 * \brief  This software allows the ESP32 microcontroller to:
 *  - Detect a pushbutton press and toggle between "active" and "inactive" states.
 *  - Publish this state to a ROS 2 topic.
 *  - Subscribe to an LED brightness control topic and adjust brightness when "active".
 *  - Use a timer to periodically publish the button state.
 *  - Ensure reliable communication with a ROS 2 agent over a wireless network.
 */

 /*
 * FEATURES:
 * - Uses external interrupts with debounce handling for button state
 * - Publishes "active" or "inactive" when button is toggled
 * - Uses a timer callback to periodically publish button state
 * - LED follows subscriber data, but only when the button is "active"
 * - Waits for connection to the ROS 2 agent before starting operations
 */

/*
 * INSTRUCTIONS:
 *  - Connect a normally open (NO) pushbutton in a pulldown resistor configuration to ButtonPin (ESP32 Pin 23).
 (https://www.electroduino.com/arduino-tutorial-8-arduino-digitalread-using-push-button/)
 *  - The LED brightness is controlled via PWM on ESP32 Pin 22.
 *  - A ROS 2 agent must be running on a host machine.
 *  - The ESP32 subscribes to "led_brightness" and publishes "button_state" every 100ms.
 */


/**
Button Pulldown Configuration

                  ----- +5
                    |
                    |
             Switch  \
               NO     \
                    |
                    |-----o (gpio 23)
                   ---
                   |r|  Resistor 10K
                   |r|
                   ---
                    |
                  __|__
                   ---   GND
                    -
*/




// ======== Include necessary Micro-ROS and ESP32 libraries ========
#include <micro_ros_arduino.h>           // Micro-ROS library for ESP32
#include <rcl/rcl.h>                     // ROS 2 client library (Core)
#include <rcl/error_handling.h>          // ROS 2 error handling utilities
#include <rclc/rclc.h>                   // Micro-ROS client library
#include <rclc/executor.h>               // Executor to handle callbacks
#include <std_msgs/msg/string.h>         // String message type for button state
#include <std_msgs/msg/float32.h>        // Float message type for LED brightness
#include <rmw_microros/rmw_microros.h>   // Middleware functions for Micro-ROS
#include <stdio.h>                       // Standard I/O for debugging
#include <micro_ros_utilities/string_utilities.h>  // Utilities for handling strings


// ======== Micro-ROS Entity Declarations ========
rclc_support_t support;       // Micro-ROS execution context
rclc_executor_t executor;     // Manages execution of tasks (timers, subscribers)
rcl_allocator_t allocator;    // Handles memory allocation

rcl_node_t node;              // Defines the ROS 2 node

rcl_publisher_t button_publisher;    // Publishes button state
rcl_subscription_t led_subscriber;   // Subscribes to LED brightness control
rcl_timer_t timer;                   // Periodically publishes button state

std_msgs__msg__String button_msg;    // Holds the button state ("active" or "inactive")
std_msgs__msg__Float32 led_msg;      // Holds the LED brightness value

// ======== Macro Definitions ========
// Error handling macros
//Executes fn and returns false if it fails.
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){return false;}}
// Executes fn, but ignores failures.
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}


// Executes a statement (X) every N milliseconds
#define EXECUTE_EVERY_N_MS(MS, X)  do { \
  static volatile int64_t init = -1; \
  if (init == -1) { init = uxr_millis();} \
  if (uxr_millis() - init > MS) { X; init = uxr_millis();} \
} while (0)\

// ======== Hardware Pin Definitions ========
#define ButtonPin  23   // Pushbutton connected to GPIO 23
#define PWM_PIN 22     // LED connected to GPIO 22 (PWM output)
#define PWM_FRQ 5000   // PWM frequency (Hz)
#define PWM_RES 8      // PWM resolution (bits)
#define PWM_CHNL 0     // PWM channel
#define MSG_MIN_VAL 0  // Minimum brightness value
#define MSG_MAX_VAL 1  // Maximum brightness value
#define DEBOUNCE_DELAY 50  // Debounce delay in milliseconds


// ======== Button Handling Variables ========
volatile bool interruptStatus = false;  // Tracks if an interrupt has occurred
volatile unsigned long lastInterruptTime = 0;  // Stores last interrupt time for debounce
bool buttonState = false;  // Stores the toggle state of the button
float pwm_set_point = 0.0;  // Stores the LED brightness value

// ======== Micro-ROS Connection State Machine ========
enum states {
  WAITING_AGENT,        // Waiting for ROS 2 agent connection
  AGENT_AVAILABLE,      // Agent detected
  AGENT_CONNECTED,      // Successfully connected
  AGENT_DISCONNECTED    // Connection lost
} state;


// ======== Function Prototypes ========
bool create_entities();
void destroy_entities();

// ======== Interrupt Service Routine (ISR) ========
// Detects button press and ensures debounce handling
void IRAM_ATTR interruptHandler() {
  unsigned long currentTime = millis();
  if ((currentTime - lastInterruptTime) > DEBOUNCE_DELAY) {  
    interruptStatus = true;
    lastInterruptTime = currentTime;
  }
}

// ======== Reads and Debounces Button Input ========
bool buttonRead() {
  static bool buttonPressed = false;
  bool currentButtonState = digitalRead(ButtonPin);  

  if (interruptStatus) {  
    if (currentButtonState) {
      buttonPressed = true;
    } 

    if (buttonPressed && !currentButtonState) {  
      buttonPressed = false;  
      interruptStatus = false;  
      return true;  
    }
  }
  return false;
}

// ======== Publishes Button State to ROS 2 ========
void publish_button_state() {
  // Use micro_ros_string_utilities_set() to safely assign the string
  button_msg.data = micro_ros_string_utilities_set(button_msg.data, buttonState ? "active" : "inactive");
  rcl_publish(&button_publisher, &button_msg, NULL);
}

// ======== Timer Callback: Publishes Button State Periodically ========
void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  (void) last_call_time;
  if (timer != NULL) {
    publish_button_state();
  }
}

// ======== Subscriber Callback: Adjusts LED Brightness ========
void subscription_callback(const void * msgin) {  
  const std_msgs__msg__Float32 * msg = (const std_msgs__msg__Float32 *)msgin;
  pwm_set_point = constrain(msg->data, MSG_MIN_VAL, MSG_MAX_VAL);

  // If button is "active", update LED brightness
  if (buttonState) {
    ledcWrite(PWM_CHNL, (uint32_t)(pow(2, PWM_RES) * (pwm_set_point / 1.0)));
  } else {
    ledcWrite(PWM_CHNL, 0); // Turn off LED if button is "inactive"
  }
}

// ======== Setup Function ========
void setup() {
  set_microros_transports();  // Initialize Micro-ROS communication
  
  // Setup GPIOs
  pinMode(ButtonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(ButtonPin), interruptHandler, CHANGE);

  // Setup LED PWM
  pinMode(PWM_PIN, OUTPUT);
  ledcSetup(PWM_CHNL, PWM_FRQ, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CHNL);       
}

// ======== Main Loop Function ========
void loop() {
  switch (state) {

    case WAITING_AGENT:
      EXECUTE_EVERY_N_MS(500, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_AVAILABLE : WAITING_AGENT;);
      break;

    case AGENT_AVAILABLE:
      state = (true == create_entities()) ? AGENT_CONNECTED : WAITING_AGENT;
      if (state == WAITING_AGENT) {
        destroy_entities();
      };
      break;

    case AGENT_CONNECTED:
      EXECUTE_EVERY_N_MS(200, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_CONNECTED : AGENT_DISCONNECTED;);
      if (state == AGENT_CONNECTED) {

          if (buttonRead()) {
            buttonState = !buttonState;  // Toggle state between active and inactive
            publish_button_state();  // Publish new state to ROS 2
            // If button is inactive, turn off LED
            if (!buttonState) {
              ledcWrite(PWM_CHNL, 0);
            }
          }

        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      }
      break;

    case AGENT_DISCONNECTED:
      destroy_entities();
      state = WAITING_AGENT;
      break;
      
    default:
      break;
  }
}

// ======== ROS 2 Entity Creation and Cleanup Functions ========
bool create_entities()
{
  // Initialize Micro-ROS
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "esp32_button_led_node", "", &support));

  // Initialize Button Publisher
  RCCHECK(rclc_publisher_init_default(
      &button_publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
      "button_state"));

  // Initialize LED Subscriber
  RCCHECK(rclc_subscription_init_default(
      &led_subscriber,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
      "led_brightness"));

  // Initialize Timer (Publishes Button State Every 2 Seconds)
  const unsigned int timer_timeout = 100;  // 0.1 seconds
  RCCHECK(rclc_timer_init_default(
      &timer,
      &support,
      RCL_MS_TO_NS(timer_timeout),
      timer_callback));

  // Initialize Executor
  // create zero initialised executor (no configured) to avoid memory problems
  executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &led_subscriber, &led_msg, &subscription_callback, ON_NEW_DATA));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  return true;
}

void destroy_entities()
{
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_subscription_fini(&led_subscriber, &node);
  rcl_publisher_fini(&button_publisher, &node);
  rcl_timer_fini(&timer);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}
