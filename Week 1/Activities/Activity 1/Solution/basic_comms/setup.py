from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'basic_comms'

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
    maintainer='mario',
    maintainer_email='mario@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talker = basic_comms.talker:main',
            'talker_old_school = basic_comms.talker_old_school:main',
            'listener = basic_comms.listener:main',
            'listener_old_school = basic_comms.listener_old_school:main'
        ],
    },
)
