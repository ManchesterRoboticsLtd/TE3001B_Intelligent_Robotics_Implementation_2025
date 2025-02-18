#IMPORTS REQUIRED TO SET THE PACKAGE ADDRESS (DIRECTORIES)
import os
from ament_index_python.packages import get_package_share_directory

#iMPORTS REQUIRED FOR CALLING OTHER LAUNCH FILES (NESTING)
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

#IMPORTS REQUIRED TO PUSH A NAMESPACE (APPEND) A NAMESPACE TO A NESTED LAUNCH FILE
from launch.actions import GroupAction
from launch_ros.actions import PushRosNamespace

#INITIATE LAUNCH FILE
def generate_launch_description():

    #USER VARIABLES
    package = 'motor_control'             #Package to be launched
    launch_file = 'motor_launch.py' #Launch file to get a namespace

    group1_ns = 'group1'    #namespace to be used for group 1
    group2_ns = 'group2'    #namespace to be used for group 2
    group3_ns = 'group3'    #namespace to be used for group 3

    #Get the address of the package 
    package_directory = get_package_share_directory(package)
    #Get the address of the launch file
    launch_file_path = os.path.join(package_directory, 'launch', launch_file)
    #Set the launch file source for the group1 and group2
    launch_source1 = PythonLaunchDescriptionSource(launch_file_path)
    launch_source2 = PythonLaunchDescriptionSource(launch_file_path)

    #Include the launch description for group1
    talker_listener_launch_1 = IncludeLaunchDescription(launch_source1)
    #Include the launch description for group2
    talker_listener_launch_2 = IncludeLaunchDescription(launch_source2)

    #Include the launch description for group3
    #THIS IS ANOTHER WAY OF DOING THE PREVIOUS STEPS (MORE COMPACT) THE RESULTS ARE THE SAME
    talker_listener_launch_3= IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory('motor_control'), 'launch'),
            '/motor_launch.py'])
    )

    #SET NAMESPACE FOR ALL THE NODES INSIDE THE LAUNCH FILE
    motor_control_group1 = GroupAction(
        actions=[PushRosNamespace(group1_ns),
                 talker_listener_launch_1,
                 ]
    )

    motor_control_group2 = GroupAction(
        actions=[PushRosNamespace(group2_ns),
                 talker_listener_launch_2,
                 ]
    )

    motor_control_group3 = GroupAction(
        actions=[PushRosNamespace(group3_ns),
                 talker_listener_launch_3,
                 ]
    )

    #LAUNCH THE DESCRIPTION
    l_d = LaunchDescription([motor_control_group1, motor_control_group2, motor_control_group3])

    return l_d