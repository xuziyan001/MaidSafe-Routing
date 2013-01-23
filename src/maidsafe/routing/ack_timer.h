/*******************************************************************************
 *  Copyright 2012 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ******************************************************************************/


#ifndef MAIDSAFE_ROUTING_ACK_TIMER_H_
#define MAIDSAFE_ROUTING_ACK_TIMER_H_

#include<mutex>
#include <map>
#include <string>
#include <utility>
#include <tuple>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio.hpp"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/non_routing_table.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/rudp/managed_connections.h"
#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/routing/utils.h"

namespace asio = boost::asio;

namespace maidsafe {

namespace routing {

typedef uint32_t AckId;

namespace protobuf { class Message;}  // namespace protobuf

typedef std::shared_ptr<asio::deadline_timer> TimerPointer;
typedef std::tuple<AckId, protobuf::Message, TimerPointer, uint16_t> Timers;
typedef std::function<void(const boost::system::error_code &error)> Handler;

class RoutingPrivate;

class AckTimer {
 public:
  explicit AckTimer(AsioService &io_service);
  ~AckTimer();
  AckId GetId();
  void Add(const protobuf::Message& message, Handler handler, int timeout);
  void Remove(const AckId& ack_id);
  void HandleAckMessage(int32_t ack_id);
  bool NeedsAck(const protobuf::Message& message, const NodeId& node_id);

  friend class RoutingPrivate;

 private:
  AckTimer &operator=(const AckTimer&);
  AckTimer(const AckTimer&);
  AckTimer(const AckTimer&&);
  AsioService& io_service_;
  AckId ack_id_;
  std::mutex mutex_;
  std::vector<Timers> queue_;
};

}  // namespace routing

}  // namespace maidsafe


#endif  // MAIDSAFE_ROUTING_ACK_TIMER_H_