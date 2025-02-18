from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    motor_node_1 = Node(name="motor_sys_1",
                       package='motor_control',
                       executable='dc_motor',
                       emulate_tty=True,
                       output='screen',
                       namespace="group1"
                       )
    
    sp_node_1 = Node(name="sp_gen_1",
                       package='motor_control',
                       executable='set_point',
                       emulate_tty=True,
                       output='screen',
                       namespace="group1"
                       )
    
    motor_node_2 = Node(name="motor_sys_2",
                       package='motor_control',
                       executable='dc_motor',
                       emulate_tty=True,
                       output='screen',
                       namespace="group2"
                       )
    
    sp_node_2 = Node(name="sp_gen_2",
                       package='motor_control',
                       executable='set_point',
                       emulate_tty=True,
                       output='screen',
                       namespace="group2"
                       )
    
    l_d = LaunchDescription([motor_node_1, sp_node_1, motor_node_2, sp_node_2])

    return l_d