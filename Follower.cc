#include "Follower.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

namespace {
  const uint64_t RECOVER_CYCLES = 8500000000ULL;
}

Follower::Follower(QuorumPeer& self, Log& log, TimerManager& tm,
                   const std::string& id) :
  self_(self), log_(log), tm_(tm), following_(false), recovering_(true)
{
  recover_timer_ = tm_.Alloc(this);
  anl_.set_from(id);
  fi_.set_from(id);
}

void
Follower::Recover(const std::string& leader)
{
  following_ = true;
  tm_.Arm(recover_timer_, RECOVER_CYCLES);
  leader_ = leader;
  fi_.set_zxid(log_.LastZxid());
  self_.Send(leader, fi_);
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
        anl_.add_zxids(nli.propose_diff(i).zxid());
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
    self_.Send(leader_, pa_);
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
Follower::Callback()
{
  following_ = false;
  recovering_ = true;
  self_.Fail();
}
