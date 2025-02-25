// Include Libraries to be used
#include <micro_ros_arduino.h>    //micro-ros-arduino library
#include <rcl/rcl.h>              //Core ROS 2 Client Library (RCL) for node management.
#include <rcl/error_handling.h>   //Error handling utilities for Micro-ROS.
#include <rclc/rclc.h>            //Micro-ROS Client library for embedded devices.
#include <rclc/executor.h>        //Micro-ROS Executor to manage callbacks
#include <std_msgs/msg/float32.h>   //Predefined ROS 2 message type
#include <rmw_microros/rmw_microros.h> //ROS Middleware for Micro-ROS, provides functions for interfacing Micro-ROS with DDS.
#include <stdio.h>                //Standard I/O library for debugging.

//Declare nodes to be used
rcl_node_t node;            //Represents a ROS 2 Node running on the microcontroller.

//Instantiate executor and its support classes
rclc_executor_t executor;   //Manages task execution (timers, callbacks, etc.).
rclc_support_t support;     //Data structure that holds the execution context of Micro-ROS, including its communication state, memory management, and initialization data.
rcl_allocator_t allocator;  //Manages memory allocation.

//Declare Subscribers to be used
rcl_subscription_t subscriber;

//Declare Messages to be used
std_msgs__msg__Float32 msg;  //Defines a message of type float32.

//Define Macros to be used
//Executes fn and returns false if it fails.
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){return false;}}
// Executes fn, but ignores failures.
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}
//  Executes a given statement (X) periodically every MS milliseconds
#define EXECUTE_EVERY_N_MS(MS, X)  do { \
  static volatile int64_t init = -1; \
  if (init == -1) { init = uxr_millis();} \
  if (uxr_millis() - init > MS) { X; init = uxr_millis();} \
} while (0)\

//Defines State Machine States
enum states {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state;

//Specifies GPIO pin 13 for controlling an LED
#define PWM_PIN 22 //DEFINE PWM_PIN
#define PWM_FRQ 5000 //Define PWM Frequency
#define PWM_RES 8  //Define PWM Resolution
#define PWM_CHNL 0    //Define Channel
#define MSG_MIN_VAL 0 //Define min input value
#define MSG_MAX_VAL 1 //Define max input value
//Variables to be used
float pwm_set_point = 0.0;

//Create entity functions
bool create_entities();
void destroy_entities();

//Define callbacks
void subscription_callback(const void * msgin)
{  
  //Get the message received and store it on the message msg
  const std_msgs__msg__Float32 * msg = (const std_msgs__msg__Float32 *)msgin;
  pwm_set_point = constrain(msg->data, MSG_MIN_VAL, MSG_MAX_VAL);
  ledcWrite(PWM_CHNL, (uint32_t) (pow(2, PWM_RES) * (pwm_set_point  / 1.0)));
}

//Setup
void setup() {
  set_microros_transports(); // Initializes communication between ESP32 and the ROS 2 agent (Serial).
  //Setup Microcontroller Pins
  pinMode(PWM_PIN, OUTPUT);
  ledcSetup(PWM_CHNL, PWM_FRQ, PWM_RES);  //Setup the PWM
  ledcAttachPin(PWM_PIN, PWM_CHNL);       //Setup Attach the Pin to the Channel   
}

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

bool create_entities()
{
   //Initializes memory allocation for Micro-ROS operations.
  allocator = rcl_get_default_allocator();

   //Creates a ROS 2 support structure to manage the execution context.
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_sub_node", "", &support));

  // create subscriber
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "micro_ros_sub"));

  // create zero initialised executor (no configured) to avoid memory problems
  executor = rclc_executor_get_zero_initialized_executor();
  // Initializes the Micro-ROS Executor, which manages tasks and callbacks.
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  // Register suscription with executor
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));

  return true;
}

void destroy_entities()
{
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_subscription_fini(&subscriber, &node);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}
