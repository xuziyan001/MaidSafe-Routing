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

#include <algorithm>
#include <memory>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/return_codes.h"
#include "maidsafe/routing/routing_pb.h"
#include "maidsafe/routing/ack_timer.h"


namespace bptime = boost::posix_time;

namespace maidsafe {

namespace routing {

namespace test {

class AckTimerTest : public testing::Test {
 public:
  AckTimerTest()
      : asio_service_(2),
        ack_timer_(asio_service_),
        call_functor_(),
        message_() {
    asio_service_.Start();


    call_functor_ = [=](const boost::system::error_code &error) {
                      if (error.value() == boost::system::errc::success) {
                        message_.set_id(message_.id() + 1);
                        ack_timer_.Add(message_, call_functor_, 2);
                      }
                    };

    message_.set_type(-200);
    message_.set_destination_id("destination_id");
    message_.set_direct(false);
    message_.add_data("response data");
    message_.set_source_id("source_id");
    message_.set_ack_id(ack_timer_.GetId());
    message_.set_id(0);
  }

 protected:
  AsioService asio_service_;
  AckTimer ack_timer_;
  Handler call_functor_;
  protobuf::Message message_;
};

TEST_F(AckTimerTest, BEH_CallOnce) {
  ack_timer_.Add(message_, call_functor_, 2);
  Sleep(boost::posix_time::seconds(3));
  ack_timer_.Remove(message_.ack_id());
  EXPECT_EQ(1, message_.id());
}

TEST_F(AckTimerTest, BEH_CallTwice) {
  ack_timer_.Add(message_, call_functor_, 2);
  Sleep(boost::posix_time::seconds(5));
  ack_timer_.Remove(message_.ack_id());
  EXPECT_EQ(2, message_.id());
}

TEST_F(AckTimerTest, BEH_CallRemove) {
  ack_timer_.Add(message_, call_functor_, 2);
  Sleep(boost::posix_time::seconds(7));
  EXPECT_EQ(2, message_.id());
}

}  // namespace test

}  // namespace routing

}  // namespace maidsafe
