###############################################################
######                                                   ######
######			Author: Pablo R.S. (aka. Bardo91)        ######
######                                                   ######
###############################################################

cmake_minimum_required (VERSION 2.8 FATAL_ERROR)
project(hecatonquiros)
set(PROJECT_VERSION 0.4.0)

# Load macros
include(cmake/hecatonquiros_macros.cmake)

# Common configuration
IF(UNIX)
	add_definitions(-std=c++11)
ENDIF()

# Add core library
find_package(catkin REQUIRED COMPONENTS
			geometry_msgs
			sensor_msgs
			std_msgs
			roscpp
			message_generation
			)
if(${catkin_FOUND})
	add_definitions(-DHAS_ROS)
endif(${catkin_FOUND})

add_service_files(
  FILES
  SetJoints.srv
  SetPose.srv
)

generate_messages(
    DEPENDENCIES std_msgs geometry_msgs sensor_msgs
)

catkin_package(
	CATKIN_DEPENDS message_runtime std_msgs geometry_msgs sensor_msgs
)

add_subdirectory(arm_controller)

# Add set of tools
file(GLOB_RECURSE TOOL_SRC_arm_joystick "tools/arm_joystick/src/*.h"  "tools/arm_joystick/src/*.inl" "tools/arm_joystick/src/*.cpp")

compile_tool(arm_joystick)

file(GLOB_RECURSE TOOL_SRC_arm_tester "tools/arm_tester/src/*.h" "tools/arm_tester/src/*.inl" "tools/arm_tester/src/*.cpp")

compile_tool(arm_tester)

file(GLOB_RECURSE TOOL_SRC_calibration_feetech "tools/calibration_feetech/src/*.h" "tools/calibration_feetech/src/*.inl" "tools/calibration_feetech/src/*.cpp")

compile_tool(calibration_feetech)

file(GLOB_RECURSE TOOL_SRC_joystick_controller "tools/joystick_controller/src/*.h" "tools/joystick_controller/src/*.inl" "tools/joystick_controller/src/*.cpp")

compile_library_tool(joystick_controller)

file(GLOB_RECURSE TOOL_SRC_manipulator_controller "tools/manipulator_controller/src/*.h" "tools/manipulator_controller/src/*.inl" "tools/manipulator_controller/src/*.cpp")

compile_tool(manipulator_controller)






