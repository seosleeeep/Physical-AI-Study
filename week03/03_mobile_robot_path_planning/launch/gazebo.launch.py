from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory

import os
import xacro


def generate_launch_description():

    pkg_mobile_robot = get_package_share_directory(
        'mobile_robot_description'
    )

    pkg_ros_gz_sim = get_package_share_directory(
        'ros_gz_sim'
    )

    xacro_file = os.path.join(
        pkg_mobile_robot,
        'urdf',
        'mobile_robot.urdf.xacro'
    )

    robot_description_config = xacro.process_file(xacro_file)

    robot_description = {
        'robot_description':
        robot_description_config.toxml()
    }


    # ==========================
    # Gazebo 실행
    # ==========================

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                pkg_ros_gz_sim,
                'launch',
                'gz_sim.launch.py'
            )
        ),
        launch_arguments={
            'gz_args': '-r empty.sdf'
        }.items()
    )


    # ==========================
    # Robot State Publisher
    # ==========================

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description]
    )


    # ==========================
    # Gazebo에 Robot Spawn
    # ==========================

    spawn_robot = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=[
            '-name',
            'mobile_robot',

            '-topic',
            'robot_description',

            '-x', '0.0',
            '-y', '0.0',
            '-z', '0.10'
        ],
        output='screen'
    )


    return LaunchDescription([
        gazebo,
        robot_state_publisher,
        spawn_robot
    ])
