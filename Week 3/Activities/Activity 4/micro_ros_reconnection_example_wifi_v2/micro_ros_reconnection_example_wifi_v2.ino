#include <micro_ros_arduino.h>  // Micro-ROS library for Arduino
#include <WiFi.h>               // Wi-Fi library for ESP32
#include <WiFiUdp.h>            // UDP communication over Wi-Fi

#include <stdio.h>              // Standard I/O library
#include <rcl/rcl.h>            // Core ROS 2 Client Library (RCL) for node management
#include <rcl/error_handling.h> // Error handling utilities
#include <rclc/rclc.h>          // Micro-ROS client library for embedded devices
#include <rclc/executor.h>      // Micro-ROS Executor to manage callbacks
#include <rmw_microros/rmw_microros.h> // ROS Middleware for Micro-ROS

#include <std_msgs/msg/int32.h>  // Predefined ROS 2 message type (integer messages)


// Macros for Error Checking
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){return false;}}  // Return false on failure
#define RMCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RMW_RET_OK)){error_loop();}}  // Enter error loop on failure
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// Macro for executing a function every N milliseconds
#define EXECUTE_EVERY_N_MS(MS, X)  do { \
  static volatile int64_t init = -1; \
  if (init == -1) { init = uxr_millis();} \
  if (uxr_millis() - init > MS) { X; init = uxr_millis();} \
} while (0)\

// Micro-ROS entities
rclc_support_t support;    // Holds the execution context of Micro-ROS
rclc_executor_t executor;  // Manages task execution (timers, callbacks, etc.)
rcl_allocator_t allocator;  // Memory allocation manager

rcl_node_t node;      // Represents a ROS 2 Node running on the microcontroller
rcl_timer_t timer;    // Timer for periodic message publishing
rcl_publisher_t publisher;  // Publisher for sending messages to ROS 2

std_msgs__msg__Int32 msg;  // Integer message type

micro_ros_agent_locator locator;    // Stores connection details for Micro-ROS Agent

// Static IP configuration for ESP32 Access Point
IPAddress local_ip = {10, 16, 1, 1};
IPAddress gateway = {10, 16, 1, 1};
IPAddress subnet = {255, 255, 255, 0};

// Wi-Fi credentials (ESP32 acting as an Access Point)
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

// Micro-ROS Agent configuration (host machine)
const char* agent_ip = "10.16.1.2";
const int agent_port = 8888;

// Enum representing different connection states of the microcontroller
enum states {
  WAITING_AGENT,       // Waiting for a connection to the Micro-ROS agent
  AGENT_AVAILABLE,     // Agent found, trying to establish communication
  AGENT_CONNECTED,     // Connected to the agent, publishing messages
  AGENT_DISCONNECTED   // Lost connection, trying to reconnect
} state;


// Function that gets called if there is a failure in initialization
void error_loop(){
  while(1){
    // Toggle LED state
      printf("Failed initialisation. Aborting.\n");  // Print error message
    // Wait for 100 milliseconds before retrying
    delay(100);
  }
}

// Timer callback function, runs periodically to publish messages
void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  (void) last_call_time;
  if (timer != NULL) {
    rcl_publish(&publisher, &msg, NULL); // Publish message to ROS 2 topic
    msg.data++; // Increment message data for the next cycle
  }
}

// Function to create Micro-ROS entities (node, publisher, timer)
bool create_entities()
{
  // Initialize Micro-ROS support
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create ROS 2 node
  RCCHECK(rclc_node_init_default(&node, "int32_publisher_rclc", "", &support));

  // Create a best-effort publisher (non-reliable, no message history) (QoS)
  RCCHECK(rclc_publisher_init_best_effort(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "std_msgs_msg_Int32"));

  // Create a timer to publish messages every 1000ms (1 second)
  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // Initialize Executor (handles timer callbacks)
  executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  return true;  // Return true if all entities are successfully created
}

// Function to clean up Micro-ROS entities when disconnected
void destroy_entities()
{
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_publisher_fini(&publisher, &node);
  rcl_timer_fini(&timer);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

// Setup function - Runs once when ESP32 starts
void setup() {

  // Initialize memory allocator
  allocator = rcl_get_default_allocator();

  // Set up Micro-ROS agent connection details
  locator.address.fromString(agent_ip);
  locator.port = agent_port;

  // Set up ESP32 as a Wi-Fi Access Point
  WiFi.mode(WIFI_AP_STA);  
  WiFi.softAP(ssid,password);
  delay(1000);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // Configure Micro-ROS transport using Wi-Fi
  RMCHECK(rmw_uros_set_custom_transport(
    false,
    (void *) &locator,
    arduino_wifi_transport_open,
    arduino_wifi_transport_close,
    arduino_wifi_transport_write,
    arduino_wifi_transport_read
  ));

  // Set initial state to waiting for ROS 2 Agent
  state = WAITING_AGENT;
  // Initialize message data
  msg.data = 0;
}

// Loop function - Runs continuously
void loop() {
  switch (state) {

    case WAITING_AGENT:
      // Try to ping the Micro-ROS agent every second
      EXECUTE_EVERY_N_MS(1000, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_AVAILABLE : WAITING_AGENT;);
      break;

    case AGENT_AVAILABLE:
      // Try to create ROS entities, move to connected state if successful
      state = (true == create_entities()) ? AGENT_CONNECTED : WAITING_AGENT;
      if (state == WAITING_AGENT) {
        destroy_entities();
      };
      break;

    case AGENT_CONNECTED:
      // Check connection every second, if lost move to disconnected state
      EXECUTE_EVERY_N_MS(1000, state = (RMW_RET_OK == rmw_uros_ping_agent(500, 1)) ? AGENT_CONNECTED : AGENT_DISCONNECTED;);
      if (state == AGENT_CONNECTED) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1));
      }
      break;

    case AGENT_DISCONNECTED:
      // Destroy entities and try reconnecting
      destroy_entities();
      state = WAITING_AGENT;
      break;
      
    default:
      break;
  }

}
