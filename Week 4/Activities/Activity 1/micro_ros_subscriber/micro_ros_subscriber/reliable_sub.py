import rclpy
from rclpy.node import Node
import numpy as np
from std_msgs.msg import Float32
from rclpy.qos import qos_profile_system_default

class MicroROSSubscriber(Node):
    def __init__(self):
        super().__init__('micro_ros_subscriber')

        self.reliable_subscription = self.create_subscription(
            Float32,
            'reliable_pub',
            self.best_effort_callback,
            qos_profile=qos_profile_system_default
        )

        self.reliable_subscription

        self.signal_msg = Float32()
        
    def best_effort_callback(self, signal_in):
        self.get_logger().info(f"Received: {signal_in.data}")

def main(args=None):
    rclpy.init(args=args)

    node = MicroROSSubscriber()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if rclpy.ok():  # Ensure shutdown is only called once
            rclpy.shutdown()
        node.destroy_node()

if __name__ == '__main__':
    main()