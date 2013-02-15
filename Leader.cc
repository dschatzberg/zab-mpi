#include "Leader.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

Leader::Leader(QuorumPeer& self, Log& log, const std::string& id)
  : self_(self), log_(log), recovering_(true), leading_(false)
{
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
  ack_count_ = 0;
  acks_.clear();
  log_.NewEpoch();
}

uint64_t
Leader::Propose(const std::string& message)
{
  if (!leading_ || recovering_) {
    throw;
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
    if (recovering_) {
      message_.mutable_new_leader_info()->set_zxid(log_.LastZxid());
      uint64_t zxid = log_.CommittedZxid();
      if (fi.zxid() <= zxid) {
        message_.mutable_new_leader_info()->set_type(Message::NewLeaderInfo::DIFF);
        for(std::map<uint64_t, std::string>::const_iterator it = log_.Diff(fi.zxid());
            it != log_.End();
            ++it) {
          Message::Entry* ep =
            message_.mutable_new_leader_info()->add_diff();
          ep->set_zxid(it->first);
          ep->set_message(it->second);
        }
      }
      self_.Send(fi.from(), message_);
    }
  }
}

void
Leader::Receive(uint64_t zxid)
{
  if (acks_.count(zxid)) {
    ++acks_[zxid];
    if ((acks_[zxid] + 1) >= self_.QuorumSize()) {
      acks_.erase(zxid);
      commit_.set_commit_zxid(zxid);
      self_.Broadcast(commit_);
      log_.Commit(zxid);
    }
  }
}

void
Leader::ReceiveAckNewLeader()
{
  if (leading_ && recovering_) {
    ack_count_++;
    if ((ack_count_ + 1) >= self_.QuorumSize()) {
      recovering_ = false;
      self_.Ready();
    }
  }
}
// void
// Leader::receiveFollower(const Follower::Info& fi)
// {
//   if (self_.getState() == Leading) {
//     if (!active_) {
//       self_.send(fi.from_, NewLeaderInfo(store_.getLastZxid()));
//       Zxid zxid = store_.lastCommittedZxid();
//       if (fi.lastZxid_ <= zxid) {
//         std::vector<Commit> diff;
//         store_.diff(diff, fi.lastZxid_);
//         self_.send(fi.from_, diff);
//       } else {
//         self_.send(fi.from_, Trunc(zxid));
//       }
//     }
//   }
// }

// void
// Leader::receiveAckNewLeader(const AckNewLeader& anl)
// {
//   if (self_.getState() == Leading) {
//     if (!active_) {
//       ackCount_++;
//       if ((ackCount_ + 1) >= self_.getQuorumSize()) {
//         active_ = true;
//         self_.ready();
//       }
//     }
//   }
// }
