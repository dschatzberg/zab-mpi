#include "CommitMsg.hpp"
#include "Leader.hpp"
#include "QuorumPeer.hpp"
#include "StableStore.hpp"

Leader::Leader(QuorumPeer& self, StableStore& store)
  : self_(self), store_(store), active_(false)
{
}

void
Leader::lead()
{
  active_ = false;
  ackCount_ = 0;
  acks_.clear();
  self_.newEpoch();
}

void
Leader::receiveFollower(const Follower::Info& fi)
{
  if (self_.getState() == Leading) {
    if (!active_) {
      self_.send(fi.from_, NewLeaderInfo(store_.getLastZxid()));
      Zxid zxid = store_.lastCommittedZxid();
      if (fi.lastZxid_ <= zxid) {
        std::vector<Commit> diff;
        store_.diff(diff, fi.lastZxid_);
        self_.send(fi.from_, diff);
      } else {
        self_.send(fi.from_, Trunc(zxid));
      }
    }
  }
}

void
Leader::receiveAckNewLeader(const AckNewLeader& anl)
{
  if (self_.getState() == Leading) {
    if (!active_) {
      ackCount_++;
      if ((ackCount_ + 1) >= self_.getQuorumSize()) {
        active_ = true;
        self_.ready();
      }
    }
  }
}

void
Leader::receiveAck(const ProposalAck& pa)
{
  if (acks_.count(pa.zxid())) {
    ++acks_[pa.zxid()];
    if ((acks_[pa.zxid()] + 1) >= self_.getQuorumSize()) {
      acks_.erase(pa.zxid());
      CommitMsg commit(self_.getId(), pa.zxid());
      self_.broadcast(commit);
      store_.commit(pa.zxid());
    }
  }
}

void
Leader::write(uint32_t val)
{
  Zxid zxid = store_.getLastZxid();
  ++zxid;
  Proposal prop(self_.getId(), val, zxid);
  acks_[zxid] = 0;
  self_.broadcast(prop);
  store_.add_proposal(prop);
}
