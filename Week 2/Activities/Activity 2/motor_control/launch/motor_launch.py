from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    motor_node = Node(name="motor_sys",
                       package='motor_control',
                       executable='dc_motor',
                       emulate_tty=True,
                       output='screen',
                       parameters=[{
                        'sample_time': 0.02,
                        'sys_gain_K': 1.78,
                        'sys_tau_T': 0.5,
                        'initial_conditions': 0.0,
                            }
                        ]
                    )
    
    sp_node = Node(name="sp_gen",
                       package='motor_control',
                       executable='set_point',
                       emulate_tty=True,
                       output='screen',
                       )
    
    l_d = LaunchDescription([motor_node, sp_node])

    return l_d