#include <stdexcept>

#include "Leader.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

namespace {
  const uint64_t RECOVER_CYCLES = 4250000000ULL;
}

Leader::Leader(QuorumPeer& self, Log& log,
               TimerManager& tm, const std::string& id)
  : self_(self), log_(log), tm_(tm), recovering_(true), leading_(false)
{
  recover_timer_ = tm_.Alloc(this);
  message_.set_type(Message::NEWLEADERINFO);
  message_.mutable_new_leader_info()->set_from(id);
  propose_.set_type(Message::PROPOSAL);
  commit_.set_type(Message::COMMIT);
}

void
Leader::Lead()
{
  leading_ = true;
  recovering_ = true;
  tm_.Arm(recover_timer_, RECOVER_CYCLES);
  ack_count_ = 0;
  acks_.clear();
  for (std::map<uint64_t, std::string>::const_iterator it =
         log_.Diff(log_.CommittedZxid());
       it != log_.End();
       it++) {
    acks_[it->first] = 0;
  }
  log_.NewEpoch();
}

uint64_t
Leader::Propose(const std::string& message)
{
  if (!leading_ || recovering_) {
    throw std::runtime_error("Not ready to handle new proposals!");
  }

  uint64_t zxid = log_.LastZxid();
  zxid++;
  log_.Accept(zxid, message);
  acks_[zxid] = 0;
  propose_.mutable_proposal()->set_zxid(zxid);
  propose_.mutable_proposal()->set_message(message);
  self_.Broadcast(propose_);

  return zxid;
}

void
Leader::Receive(const Message::FollowerInfo& fi)
{
  if (leading_) {
    message_.mutable_new_leader_info()->set_zxid(log_.LastZxid());
    uint64_t zxid = log_.CommittedZxid();
    if (fi.zxid() <= zxid) {
      message_.mutable_new_leader_info()->set_type(Message::NewLeaderInfo::DIFF);
      for(std::map<uint64_t, std::string>::const_iterator it = log_.Diff(fi.zxid());
          it != log_.EndCommit();
          ++it) {
        Message::Entry* ep =
          message_.mutable_new_leader_info()->add_commit_diff();
        ep->set_zxid(it->first);
        ep->set_message(it->second);
      }
      for (std::map<uint64_t, std::string>::const_iterator it = log_.Diff(zxid);
           it != log_.End();
           ++it) {
        Message::Entry* ep =
          message_.mutable_new_leader_info()->add_propose_diff();
        ep->set_zxid(it->first);
        ep->set_message(it->second);
      }
    } else {
      throw std::runtime_error("Unhandled recovery");
    }
    self_.Send(fi.from(), message_);
  }
}

void
Leader::Receive(uint64_t zxid)
{
  if (leading_ && !recovering_) {
    if (acks_.count(zxid)) {
      ++acks_[zxid];
      if ((acks_[zxid] + 1) >= (uint32_t)self_.QuorumSize()) {
        acks_.erase(zxid);
        commit_.set_commit_zxid(zxid);
        self_.Broadcast(commit_);
        log_.Commit(zxid);
      }
    }
  }
}

void
Leader::ReceiveAckNewLeader(const Message& acks)
{
  if (leading_ && recovering_) {
    ack_count_++;
    for (int i = 0; i < acks.ack_new_leader_zxids_size(); i++) {
      Receive(acks.ack_new_leader_zxids(i));
    }
    if ((ack_count_ + 1) >= (uint32_t)self_.QuorumSize()) {
      recovering_ = false;
      tm_.Disarm(recover_timer_);
      self_.Ready();
    }
  }
}

void
Leader::Callback()
{
  leading_ = false;
  recovering_ = true;
  self_.Fail();
}
