# Imports
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32

#Class Definition
class DCMotor(Node):
    def __init__(self):
        super().__init__('dc_motor')

        # DC Motor Parameters
        self.sample_time = 0.02
        self.param_K = 1.75
        self.param_T = 0.5
        self.initial_conditions = 0.0

        #Set the messages
        self.motor_output_msg = Float32()

        #Set variables to be used
        self.input_u = 0.0
        self.output_y = self.initial_conditions
    
        #Declare publishers, subscribers and timers
        self.motor_input_sub = self.create_subscription(Float32, 'motor_input_u', self.input_callback,10)
        self.motor_speed_pub = self.create_publisher(Float32, 'motor_speed_y', 10)
        self.timer = self.create_timer(self.sample_time, self.timer_cb) 

        #Node Started
        self.get_logger().info('Dynamical System Node Started \U0001F680')   
        
    #Timer Callback
    def timer_cb(self):         
        #DC Motor Simulation
        #DC Motor Equation 𝑦[𝑘+1] = 𝑦[𝑘] + ((−1/𝜏) 𝑦[𝑘] + (𝐾/𝜏) 𝑢[𝑘]) 𝑇_𝑠
        self.output_y += (-1.0/self.param_T * self.output_y + self.param_K/self.param_T * self.input_u) * self.sample_time 
        #Publish the result
        self.motor_output_msg.data = self.output_y
        self.motor_speed_pub.publish(self.motor_output_msg)

    #Subscriber Callback
    def input_callback(self, input_sgn):
        self.input_u = input_sgn.data

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
