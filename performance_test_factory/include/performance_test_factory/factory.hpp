/* Software License Agreement (BSD License)
 *
 *  Copyright (c) 2019, iRobot ROS
 *  All rights reserved.
 *
 *  This file is part of ros2-performance, which is released under BSD-3-Clause.
 *  You may use, distribute and modify this code under the BSD-3-Clause license.
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <type_traits>

#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "performance_test/utils/names_utilities.hpp"

#include "performance_test_factory/load_plugins.hpp"
#include "nlohmann/json.hpp"

#include "performance_test/executors.hpp"
#include "performance_test/performance_node.hpp"
#include "performance_test/node_types.hpp"

namespace performance_test
{

class TemplateFactory
{
public:

  TemplateFactory(
    bool use_ipc = true,
    bool use_ros_params = true,
    bool verbose_mode = false,
    const std::string & ros2_namespace = "",
    NodeType node_type = RCLCPP_NODE);

  /**
   * Helper functions for creating several nodes at the same time.
   * These nodes will have names as "node_X", "node_Y"
   * where X, Y, etc, are all the numbers spanning from `start_id` to `end_id`.
   */
  std::shared_ptr<PerformanceNodeBase> create_node(
    const std::string & name,
    bool use_ipc = true,
    bool use_ros_params = true,
    bool verbose = false,
    const std::string & ros2_namespace = "",
    int executor_id = 0);

  std::vector<std::shared_ptr<PerformanceNodeBase>> create_subscriber_nodes(
    int start_id,
    int end_id,
    int n_publishers,
    const std::string & msg_type,
    msg_pass_by_t msg_pass_by,
    const Tracker::TrackingOptions& tracking_options = Tracker::TrackingOptions(),
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  std::vector<std::shared_ptr<PerformanceNodeBase>> create_periodic_publisher_nodes(
    int start_id,
    int end_id,
    float frequency,
    const std::string & msg_type,
    msg_pass_by_t msg_pass_by,
    size_t msg_size = 0,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  std::vector<std::shared_ptr<PerformanceNodeBase>> create_periodic_client_nodes(
    int start_id,
    int end_id,
    int n_services,
    float frequency,
    const std::string & srv_type,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  std::vector<std::shared_ptr<PerformanceNodeBase>> create_server_nodes(
    int start_id,
    int end_id,
    const std::string & srv_type,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  /**
   * Helper functions that, given a node and a std::string describing the msg_type,
   * create the publisher/subscriber/client/server accordingly
   */

  void add_subscriber_from_strings(
    std::shared_ptr<PerformanceNodeBase> n,
    const std::string & msg_type,
    const std::string & topic_name,
    const Tracker::TrackingOptions & tracking_options,
    msg_pass_by_t msg_pass_by = PASS_BY_SHARED_PTR,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  void add_periodic_publisher_from_strings(
    std::shared_ptr<PerformanceNodeBase> n,
    const std::string & msg_type,
    const std::string & topic_name,
    msg_pass_by_t msg_pass_by = PASS_BY_UNIQUE_PTR,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default,
    std::chrono::microseconds period = std::chrono::milliseconds(10),
    size_t msg_size = 0);

  void add_periodic_client_from_strings(
    std::shared_ptr<PerformanceNodeBase> n,
    const std::string & srv_type,
    const std::string & service_name,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default,
    std::chrono::microseconds period = std::chrono::milliseconds(10));

  void add_server_from_strings(
    std::shared_ptr<PerformanceNodeBase> n,
    const std::string & srv_type,
    const std::string & service_name,
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default);

  /**
   * Helper function that, given a given a json file describing a system,
   * parses it and creates the nodes accordingly
   */

  std::vector<std::shared_ptr<PerformanceNodeBase>> parse_topology_from_json(
    const std::string & json_path,
    const Tracker::TrackingOptions & tracking_options = Tracker::TrackingOptions());

private:

  std::shared_ptr<PerformanceNodeBase> create_node_from_json(
    const nlohmann::json& node_json,
    const std::string & suffix = "");

  void create_node_entities_from_json(
    std::shared_ptr<PerformanceNodeBase> node,
    const nlohmann::json & node_json,
    const Tracker::TrackingOptions & tracking_options = Tracker::TrackingOptions());

  void add_periodic_publisher_from_json(
    std::shared_ptr<PerformanceNodeBase> node,
    const nlohmann::json & pub_json);

  void add_subscriber_from_json(
    std::shared_ptr<PerformanceNodeBase> node,
    const nlohmann::json & sub_json,
    const Tracker::TrackingOptions& t_options);

  void add_periodic_client_from_json(
    std::shared_ptr<PerformanceNodeBase> node,
    const nlohmann::json & client_json);

  void add_server_from_json(
    std::shared_ptr<PerformanceNodeBase> node,
    const nlohmann::json & server_json);

  rmw_qos_profile_t get_qos_from_json(const nlohmann::json & entity_json);

  msg_pass_by_t get_msg_pass_by_from_json(const nlohmann::json & entity_json, msg_pass_by_t default_value);

  bool _use_ipc;
  bool _use_ros_params;
  bool _verbose_mode;
  std::string _ros2_namespace;
  NodeType _node_type;
};

}
