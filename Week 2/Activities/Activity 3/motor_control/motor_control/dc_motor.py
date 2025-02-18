# Imports
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32
from rcl_interfaces.msg import SetParametersResult
from custom_interfaces.srv import SetProcessBool


#Class Definition
class DCMotor(Node):
    def __init__(self):
        super().__init__('dc_motor')

        # Declare parameters
        # System sample time in seconds
        self.declare_parameter('sample_time', 0.02)
        # System gain K
        self.declare_parameter('sys_gain_K', 1.75)
        # System time constant Tau
        self.declare_parameter('sys_tau_T', 0.5)
        # System initial conditions
        self.declare_parameter('initial_conditions', 0.0)

        # DC Motor Parameters
        self.sample_time = self.get_parameter('sample_time').value
        self.param_K = self.get_parameter('sys_gain_K').value
        self.param_T = self.get_parameter('sys_tau_T').value
        self.initial_conditions = self.get_parameter('initial_conditions').value


        #Set the messages
        self.motor_output_msg = Float32()

        #Set variables to be used
        self.input_u = 0.0
        self.output_y = self.initial_conditions
        self.simulation_running = False 
    
        #Declare publishers, subscribers and timers
        self.motor_input_sub = self.create_subscription(Float32, 'motor_input_u', self.input_callback,10)
        self.motor_speed_pub = self.create_publisher(Float32, 'motor_speed_y', 10)
        self.timer = self.create_timer(self.sample_time, self.timer_cb) 

        #Parameter Callback
        self.add_on_set_parameters_callback(self.parameters_callback)
        
        #Set Server callback
        self.srv = self.create_service(SetProcessBool, 'EnableProcess', self.simulation_service_callback)

        #Node Started
        self.get_logger().info('Dynamical System Node Started \U0001F680')   
        
    #Timer Callback
    def timer_cb(self):
        
        if not self.simulation_running:
            return  # Stop processing if simulation is not running         
        
        #DC Motor Simulation
        #DC Motor Equation 𝑦[𝑘+1] = 𝑦[𝑘] + ((−1/𝜏) 𝑦[𝑘] + (𝐾/𝜏) 𝑢[𝑘]) 𝑇_𝑠
        self.output_y += (-1.0/self.param_T * self.output_y + self.param_K/self.param_T * self.input_u) * self.sample_time 
        #Publish the result
        self.motor_output_msg.data = self.output_y
        self.motor_speed_pub.publish(self.motor_output_msg)

    #Subscriber Callback
    def input_callback(self, input_sgn):
        self.input_u = input_sgn.data

    # Service Callback to Start/Stop Simulation
    def simulation_service_callback(self, request, response):
        if request.enable:
            self.simulation_running = True
            self.get_logger().info("🚀 Simulation Started")
            response.success = True
            response.message = "Simulation Started Successfully"
        else:
            self.simulation_running = False
            self.get_logger().info("🛑 Simulation Stopped")
            response.success = True
            response.message = "Simulation Stoped Successfully"

        return response

    def parameters_callback(self, params):
        for param in params:
            #system gain parameter check
            if param.name == "sys_gain_K":
                #check if it is negative
                if (param.value < 0.0):
                    self.get_logger().warn("Invalid sys_gain_K! It cannot be negative.")
                    return SetParametersResult(successful=False, reason="sys_gain_K cannot be negative")
                else:
                    self.param_K = param.value  # Update internal variable
                    self.get_logger().info(f"sys_gain_K updated to {self.param_K}")
                
            #system gain parameter check
            if param.name == "sys_tau_T":
                #check if it is negative
                if (param.value < 0.0):
                    self.get_logger().warn("Invalid sys_tau_T! It cannot be negative.")
                    return SetParametersResult(successful=False, reason="sys_tau_T cannot be negative")
                else:
                    self.param_T = param.value  # Update internal variable
                    self.get_logger().info(f"sys_tau_T updated to {self.param_T}")

        return SetParametersResult(successful=True)


#Main
def main(args=None):
    rclpy.init(args=args)

    node = DCMotor()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()

#Execute Node
if __name__ == '__main__':
    main()
