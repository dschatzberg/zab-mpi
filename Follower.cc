#include "Follower.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

namespace {
  const uint64_t RECOVER_CYCLES = 850000000ULL;
}

Follower::Follower(QuorumPeer& self, Log& log, TimerManager& tm,
                   const std::string& id) :
  self_(self), log_(log), tm_(tm), id_(id),
  following_(false), recovering_(true)
{
  recover_timer_ = tm_.Alloc(this);
  anl_.set_from(id);
  fi_.set_from(id);
}

void
Follower::Recover(const std::string& leader)
{
  following_ = true;
  leader_ = leader;

  if (leader != id_) {
    tm_.Arm(recover_timer_, RECOVER_CYCLES);
    fi_.set_zxid(log_.LastZxid());
    self_.Send(leader, fi_);
  } else {
    recovering_ = false;
  }
}

void
Follower::Receive(const NewLeaderInfo& nli)
{
  if (following_ && recovering_) {
    switch(nli.type()) {
    case NewLeaderInfo::DIFF:
      for (int i = 0; i < nli.commit_diff_size(); i++) {
        log_.Accept(nli.commit_diff(i).zxid(), nli.commit_diff(i).message());
        log_.Commit(nli.commit_diff(i).zxid());
      }
      for (int i = 0; i < nli.propose_diff_size(); i++) {
        log_.Accept(nli.propose_diff(i).zxid(), nli.propose_diff(i).message());
      }
      break;
    }
    self_.Send(leader_, anl_);
    recovering_ = false;
    tm_.Disarm(recover_timer_);
    self_.Ready();
  }
}

void
Follower::Receive(const Proposal& p)
{
  if (following_ && !recovering_) {
    log_.Accept(p.proposal().zxid(), p.proposal().message());
    pa_.set_zxid(p.proposal().zxid());
    self_.Broadcast(pa_);
  }
}

void
Follower::Receive(const Commit& c)
{
  if (following_ && !recovering_) {
    log_.Commit(c.zxid());
  }
}

void
Follower::Receive(uint32_t count, uint64_t zxid)
{
  if (following_ && !recovering_) {
    if (count >= static_cast<uint32_t>(self_.QuorumSize())) {
      log_.Commit(zxid);
    }
  }
}

void
Follower::Callback()
{
  following_ = false;
  recovering_ = true;
  self_.Fail();
}
