set(TRAJ_PROCESSING_SRCS src/trajectory_tools.cpp
                         src/time_optimal_trajectory_generation.cpp)
if(MOVEIT_BUILD_RUCKIG)
  list(APPEND TRAJ_PROCESSING_SRCS src/ruckig_traj_smoothing.cpp)
endif()

add_library(moveit_trajectory_processing SHARED ${TRAJ_PROCESSING_SRCS})
target_include_directories(
  moveit_trajectory_processing
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include/moveit_core>)
set_target_properties(moveit_trajectory_processing
                      PROPERTIES VERSION "${${PROJECT_NAME}_VERSION}")
if(MOVEIT_BUILD_RUCKIG)
  target_compile_definitions(moveit_trajectory_processing
                             PUBLIC MOVEIT_HAS_RUCKIG)
endif()
ament_target_dependencies(
  moveit_trajectory_processing
  rclcpp
  rmw_implementation
  urdf
  urdfdom
  urdfdom_headers
  visualization_msgs
  Boost)
target_link_libraries(
  moveit_trajectory_processing moveit_robot_state moveit_robot_trajectory
  $<$<BOOL:${MOVEIT_BUILD_RUCKIG}>:ruckig::ruckig>)

install(
  DIRECTORY include/
  DESTINATION include/moveit_core
  PATTERN "ruckig_traj_smoothing.hpp" EXCLUDE
  PATTERN "ruckig_traj_smoothing.h" EXCLUDE)
if(MOVEIT_BUILD_RUCKIG)
  install(FILES include/moveit/trajectory_processing/ruckig_traj_smoothing.hpp
                include/moveit/trajectory_processing/ruckig_traj_smoothing.h
          DESTINATION include/moveit_core/moveit/trajectory_processing)
endif()

if(BUILD_TESTING AND NOT APPLE)
  find_package(ament_cmake_google_benchmark REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(benchmark REQUIRED)
  if(WIN32)
    # TODO add windows paths set(APPEND_LIBRARY_DIRS
    # "$<TARGET_FILE_DIR:${PROJECT_NAME}>;$<TARGET_FILE_DIR:${PROJECT_NAME}_TestPlugins1>")
  else()
    set(APPEND_LIBRARY_DIRS
        "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_BINARY_DIR}/../robot_trajectory;${CMAKE_CURRENT_BINARY_DIR}/../utils"
    )
  endif()

  ament_add_gtest(test_time_optimal_trajectory_generation
                  test/test_time_optimal_trajectory_generation.cpp)
  target_link_libraries(test_time_optimal_trajectory_generation
                        moveit_test_utils moveit_trajectory_processing)

  if(MOVEIT_BUILD_RUCKIG)
    ament_add_gtest(test_ruckig_traj_smoothing
                    test/test_ruckig_traj_smoothing.cpp)
    target_link_libraries(test_ruckig_traj_smoothing
                          moveit_trajectory_processing moveit_test_utils)
  endif()

  ament_add_google_benchmark(robot_trajectory_benchmark
                             test/robot_trajectory_benchmark.cpp)
  target_link_libraries(
    robot_trajectory_benchmark moveit_robot_model moveit_test_utils
    moveit_robot_state moveit_robot_trajectory moveit_trajectory_processing)
endif()
