from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'micro_ros_subscriber'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob(os.path.join('launch', '*launch.[pxy][yma]*'))),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='mcr2',
    maintainer_email='mcr2@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'best_effort_sub = micro_ros_subscriber.best_effort_sub:main',
            'reliable_sub = micro_ros_subscriber.reliable_sub:main'
        ],
    },
)
