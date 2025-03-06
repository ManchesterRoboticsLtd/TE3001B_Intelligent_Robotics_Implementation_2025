from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    micro_ros_agent = Node(name="micro_ros_agent",
                       package='micro_ros_agent',
                       executable='micro_ros_agent',
                       output='screen',
                       arguments=[
                        'serial',
                        "--dev", '/dev/ttyUSB0',
                        ]
                       )
    
    sub_node = Node(name="sub_node",
                    package='micro_ros_subscriber',
                    executable='reliable_sub',
                    output='screen'
                    )
    
    rqt_node = Node(name='rqt_plot',
                    package='rqt_plot',
                    executable='rqt_plot',
                    arguments=['/reliable_pub/data']
                    )
    
    l_d = LaunchDescription([micro_ros_agent, sub_node, rqt_node ])

    return l_d