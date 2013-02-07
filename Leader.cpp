#include "Leader.hpp"
#include "QuorumPeer.hpp"
#include "StableStore.hpp"

Leader::Leader(QuorumPeer& self, StableStore& store)
  : self_(self), store_(store)
{
}

void
Leader::lead()
{
  self_.newEpoch();
}

void
Leader::receiveFollower(const Follower::Info& fi)
{
  self_.send(fi.from_, NewLeaderInfo(self_.getLastZxid()));
  Zxid zxid = store_.lastCommittedZxid();
  if (fi.lastZxid_ <= zxid) {
    std::vector<Commit> diff;
    store_.diff(diff, fi.lastZxid_);
    self_.send(fi.from_, diff);
  } else {
    self_.send(fi.from_, Trunc(zxid));
  }
}

void
Leader::receiveAckNewLeader(const AckNewLeader& anl)
{
}
