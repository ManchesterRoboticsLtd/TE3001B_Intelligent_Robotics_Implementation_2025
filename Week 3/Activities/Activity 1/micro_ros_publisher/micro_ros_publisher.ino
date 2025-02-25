// Include Libraries to be used
#include <micro_ros_arduino.h>    //micro-ros-arduino library
#include <rcl/rcl.h>              //Core ROS 2 Client Library (RCL) for node management.
#include <rcl/error_handling.h>   //Error handling utilities for Micro-ROS.
#include <rclc/rclc.h>            //Micro-ROS Client library for embedded devices.
#include <rclc/executor.h>        //Micro-ROS Executor to manage callbacks
#include <std_msgs/msg/int32.h>   //Predefined ROS 2 message type
#include <stdio.h>                //Standard I/O library for debugging.

//Declare nodes to be used
rcl_node_t node;            //Represents a ROS 2 Node running on the microcontroller.

//Instantiate executor and its support classes
rclc_executor_t executor;   //Manages task execution (timers, callbacks, etc.).
rclc_support_t support;     //Handles initialization & communication setup.
rcl_allocator_t allocator;  //Manages memory allocation.

//Declare Publishers to be used
rcl_publisher_t publisher;  //Declares a ROS 2 publisher for sending messages.

//Declare timers to be used
rcl_timer_t timer;          //Creates a timer to execute functions at intervals.

//Declare Messages to be used
std_msgs__msg__Int32 msg;  //Defines a message of type int32.

//Define Macros to be used
//Executes fn and calls error_loop() if it fails.
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
// Executes fn, but ignores failures.
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

//Specifies GPIO pin 13 for controlling an LED
#define LED_PIN 22

//Define Error Functions
void error_loop(){
  while(1){
    // Toggle LED state
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    // Wait 100 milliseconds
    delay(100);
  }
}

//Define callbacks
void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{  
  RCLC_UNUSED(last_call_time);  //Prevents compiler warnings about an unused parameter.
  if (timer != NULL) {          //Ensures the timer event is valid before executing actions.
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL)); //Publishes msg to the ROS 2 topic.
    msg.data++;         //Increments the integer message value after each publication.
  }
}

//Setup
void setup() {
  // Initializes communication between ESP32 and the ROS 2 agent (Serial).
  set_microros_transports();
  
  //Setup Microcontroller Pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  
  
  //Connection delay
  delay(2000);

  //Initializes memory allocation for Micro-ROS operations.
  allocator = rcl_get_default_allocator();

  //Creates a ROS 2 support structure to manage the execution context.
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_pub_node", "", &support));

  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_counter"));

  // create timer,
  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // Initializes the Micro-ROS Executor, which manages tasks and callbacks.
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  // Register timer with executor
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  // Initialise message
  msg.data = 0;
}

void loop() {
  //Executor Spin
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}




