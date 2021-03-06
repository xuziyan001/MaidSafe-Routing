/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/routing/group_change_handler.h"

#include <string>
#include <vector>
#include <algorithm>

#include "maidsafe/common/log.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/routing/client_routing_table.h"
#include "maidsafe/routing/message_handler.h"
#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/rpcs.h"
#include "maidsafe/routing/utils.h"


namespace maidsafe {

namespace routing {

GroupChangeHandler::GroupChangeHandler(RoutingTable& routing_table,
                                       ClientRoutingTable& client_routing_table,
                                       NetworkUtils& network)
  : routing_table_(routing_table),
    client_routing_table_(client_routing_table),
    network_(network) {}

GroupChangeHandler::~GroupChangeHandler() {}

void GroupChangeHandler::ClosestNodesUpdate(protobuf::Message& message) {
  if (message.destination_id() != routing_table_.kNodeId().string()) {
    // Message not for this node and we should not pass it on.
    LOG(kError) << "Message not for this node.";
    message.Clear();
    return;
  }
  protobuf::ClosestNodesUpdate closest_node_update;
  if (!closest_node_update.ParseFromString(message.data(0))) {
    LOG(kError) << "No Data.";
    return;
  }

  if (closest_node_update.node().empty() || !CheckId(closest_node_update.node())) {
    LOG(kError) << "Invalid node id provided.";
    return;
  }

  std::vector<NodeInfo> closest_nodes;
  NodeInfo node_info;
  for (const auto& basic_info : closest_node_update.nodes_info()) {
    if (CheckId(basic_info.node_id())) {
      node_info.node_id = NodeId(basic_info.node_id());
      node_info.rank = basic_info.rank();
      closest_nodes.push_back(node_info);
    }
  }
  assert(!closest_nodes.empty());
  UpdateGroupChange(NodeId(closest_node_update.node()), closest_nodes);
  if (!routing_table_.client_mode())
    message.Clear();
}

void GroupChangeHandler::UpdateGroupChange(const NodeId& node_id,
                                           std::vector<NodeInfo> close_nodes) {
  if (routing_table_.Contains(node_id)) {
    LOG(kVerbose) << DebugId(routing_table_.kNodeId()) << " UpdateGroupChange for "
                  << DebugId(node_id) << " size of update: " << close_nodes.size();
    routing_table_.GroupUpdateFromConnectedPeer(node_id, close_nodes);
  } else {
    LOG(kVerbose) << DebugId(routing_table_.kNodeId()) << "UpdateGroupChange for failed"
                  << DebugId(node_id) << " size of update: " << close_nodes.size();
  }
}

void GroupChangeHandler::SendClosestNodesUpdateRpcs(std::vector<NodeInfo> closest_nodes) {
  NodeId node_id(routing_table_.kNodeId());
  closest_nodes.erase(std::remove_if(closest_nodes.begin(),
                                     closest_nodes.end(),
                                     [node_id] (const NodeInfo& node_info) {
                                       return node_info.node_id == node_id;
                                     }), closest_nodes.end());
  if (closest_nodes.size() < Parameters::closest_nodes_size)
    return;
  LOG(kVerbose) << "["  << DebugId(routing_table_.kNodeId())
                << "] SendClosestNodesUpdateRpcs: " << closest_nodes.size();
  std::vector<NodeInfo> update_subscribers(closest_nodes);
  // clients are also notified of changes in connected close nodes
  for (auto& client : client_routing_table_.nodes_)
    update_subscribers.push_back(client);
  for (auto& update_subscriber : update_subscribers) {
    LOG(kVerbose) << "[" << DebugId(routing_table_.kNodeId())
                  << "] Sending update to: "
                  << DebugId(update_subscriber.node_id);
    protobuf::Message closest_nodes_update_rpc(rpcs::ClosestNodesUpdate(
        update_subscriber.node_id, routing_table_.kNodeId(), closest_nodes));
    network_.SendToDirect(closest_nodes_update_rpc, update_subscriber.node_id,
                          update_subscriber.connection_id);
  }
}

bool GroupChangeHandler::GetNodeInfo(const NodeId& node_id, const NodeId& connection_id,
                                     NodeInfo& out_node_info) {
  if (routing_table_.GetNodeInfo(node_id, out_node_info))
    return true;
  auto nodes_info(client_routing_table_.GetNodesInfo(node_id));
  for (const auto& node_info : nodes_info) {
    if (node_info.connection_id == connection_id) {
      out_node_info = node_info;
      return true;
    }
  }
  return false;
}

}  // namespace routing

}  // namespace maidsafe
