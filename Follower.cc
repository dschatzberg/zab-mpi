#include "Follower.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

namespace {
  const long RECOVER_CYCLES = 100000000;
}

Follower::Follower(QuorumPeer& self, Log& log, TimerManager& tm,
                   const std::string& id) :
  self_(self), log_(log), tm_(tm), following_(false), recovering_(true)
{
  recover_timer_ = tm_.Alloc(this);
  message_.set_type(Message::FOLLOWERINFO);
  message_.mutable_follower_info()->set_from(id);
  ack_.set_type(Message::ACKNEWLEADER);
  ack_propose_.set_type(Message::ACKPROPOSAL);
}

void
Follower::Recover(const std::string& leader)
{
  following_ = true;
  tm_.Arm(recover_timer_, RECOVER_CYCLES);
  leader_ = leader;
  message_.mutable_follower_info()->set_zxid(log_.LastZxid());
  self_.Send(leader, message_);
}

void
Follower::Receive(const Message::NewLeaderInfo& nli)
{
  if (following_ && recovering_) {
    switch(nli.type()) {
    case Message::NewLeaderInfo::DIFF:
      for (int i = 0; i < nli.commit_diff_size(); i++) {
        log_.Accept(nli.commit_diff(i).zxid(), nli.commit_diff(i).message());
      }
      for (int i = 0; i < nli.commit_diff_size(); i++) {
        log_.Commit(nli.commit_diff(i).zxid());
      }
      for (int i = 0; i < nli.propose_diff_size(); i++) {
        log_.Accept(nli.propose_diff(i).zxid(), nli.propose_diff(i).message());
        ack_.add_ack_new_leader_zxids(nli.propose_diff(i).zxid());
      }
      break;
    }
    self_.Send(leader_, ack_);
    recovering_ = false;
    tm_.Disarm(recover_timer_);
    self_.Ready();
  }
}

void
Follower::Receive(const Message::Entry& proposal)
{
  if (following_ && !recovering_) {
    log_.Accept(proposal.zxid(), proposal.message());
    ack_propose_.set_ack_zxid(proposal.zxid());
    self_.Send(leader_, ack_propose_);
  }
}

void
Follower::Receive(uint64_t zxid)
{
  if (following_ && !recovering_) {
    log_.Commit(zxid);
  }
}

void
Follower::Callback()
{
  following_ = false;
  recovering_ = true;
  self_.Fail();
}
