/* Software License Agreement (BSD License)
 *
 *  Copyright (c) 2019, iRobot ROS
 *  All rights reserved.
 *
 *  This file is part of ros2-performance, which is released under BSD-3-Clause.
 *  You may use, distribute and modify this code under the BSD-3-Clause license.
 */

#include <sys/wait.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "performance_metrics/resource_usage_logger.hpp"
#include "performance_test/communication.hpp"
#include "performance_test/performance_node.hpp"
#include "performance_test/system.hpp"
#include "performance_test/utils/fork_process.hpp"
#include "performance_test_factory/cli_options.hpp"
#include "performance_test_factory/factory.hpp"
#include "performance_test_factory/node_types.hpp"

using namespace std::chrono_literals;

static
performance_test_factory::Options
parse_options(int argc, char ** argv)
{
  auto non_ros_args = rclcpp::remove_ros_arguments(argc, argv);
  std::vector<char *> non_ros_args_c_strings;
  for (auto & arg : non_ros_args) {
    non_ros_args_c_strings.push_back(&arg.front());
  }
  int non_ros_argc = static_cast<int>(non_ros_args_c_strings.size());
  auto options = performance_test_factory::Options(non_ros_argc, non_ros_args_c_strings.data());

  return options;
}

static
std::string
create_result_directory(const std::string & topology_json)
{
  const size_t last_slash =
    topology_json.find_last_of("/");
  const std::string topology_basename =
    topology_json.substr(last_slash + 1, topology_json.length());
  const std::string result_dir_name =
    topology_basename.substr(0, topology_basename.length() - 5) + "_log";

  const std::string make_dir_cmd = "mkdir -p " + result_dir_name;
  const int ret = system(make_dir_cmd.c_str());
  assert(ret == 0);

  return result_dir_name;
}

static
std::unique_ptr<performance_test::System>
create_ros2_system(
  const performance_test_factory::Options & options,
  const std::string & events_output_path)
{
  std::optional<std::string> events_output_path_opt;
  if (options.tracking_options.is_enabled) {
    events_output_path_opt = events_output_path;
  }
  auto system = std::make_unique<performance_test::System>(
    static_cast<performance_test::ExecutorType>(options.executor),
    performance_test::SpinType::SPIN,
    events_output_path_opt);

  return system;
}

static
std::vector<performance_test::PerformanceNodeBase::SharedPtr>
create_ros2_nodes(
  const performance_test_factory::Options & options,
  const std::string & topology_json)
{
  // Get the node type from options
  auto node_type = static_cast<performance_test_factory::NodeType>(options.node);
  // Load topology from json file
  auto factory = performance_test_factory::TemplateFactory(
    options.ipc,
    options.ros_params,
    false,
    "",
    node_type);

  auto nodes_vec = factory.parse_topology_from_json(
    topology_json,
    options.tracking_options);

  return nodes_vec;
}

int main(int argc, char ** argv)
{
  auto options = parse_options(argc, argv);
  std::cout << options << "\n" << "Start test!" << std::endl;

  // Fork processes and select topology json for this process
  pid_t pid = getpid();
  size_t process_index = performance_test::fork_process(options.topology_json_list.size());
  std::string topology_json = options.topology_json_list[process_index];

  // Create results dir based on the topology name
  std::string result_dir_name = create_result_directory(topology_json);
  // Define output paths
  std::string resources_output_path = result_dir_name + "/resources.txt";
  std::string events_output_path = result_dir_name + "/events.txt";
  std::string latency_all_output_path = result_dir_name + "/latency_all.txt";
  std::string latency_total_output_path = result_dir_name + "/latency_total.txt";

  // Start resources logger
  performance_metrics::ResourceUsageLogger ru_logger(resources_output_path);
  ru_logger.start(std::chrono::milliseconds(options.resources_sampling_per_ms));

  // Start ROS 2 context
  rclcpp::init(argc, argv);

  // Create ROS 2 nodes
  auto nodes_vec = create_ros2_nodes(options, topology_json);

  // Create ROS 2 system manager
  auto ros2_system = create_ros2_system(options, events_output_path);
  ros2_system->add_nodes(nodes_vec);

  // now the system is complete and we can make it spin for the requested duration
  bool wait_for_discovery = false;
  ros2_system->spin(
    std::chrono::seconds(options.duration_sec),
    wait_for_discovery,
    options.name_threads);

  // terminate the experiment
  ru_logger.stop();
  rclcpp::shutdown();
  std::this_thread::sleep_for(500ms);

  ros2_system->log_latency_all_stats();

  if (options.topology_json_list.size() > 1) {
    std::cout << std::endl << "Process total:" << std::endl;
    ros2_system->log_latency_total_stats();
  }

  std::cout << std::endl;
  ros2_system->save_latency_all_stats(latency_all_output_path);
  ros2_system->save_latency_total_stats(latency_total_output_path);

  // Parent process: wait for children to exit and print system stats
  if (pid != 0) {
    if (options.topology_json_list.size() > 1) {
      waitpid(getpid() + 1, &pid, 0);
    }
    std::cout << "System total:" << std::endl;
    ros2_system->print_aggregate_stats(options.topology_json_list);
  }
}
