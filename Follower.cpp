#include "Follower.hpp"
#include "PeerId.hpp"
#include "QuorumPeer.hpp"
#include "StableStore.hpp"

Follower::Follower(QuorumPeer& self, StableStore& store) :
  self_(self), store_(store)
{
}

void
Follower::recover(PeerId leader)
{
  //FIXME: setup a timeout
  leader_ = leader;
  self_.send(leader, Info(self_.getId(), store_.getLastZxid()));
}

void
Follower::receiveLeader(const NewLeaderInfo& info)
{
}

void
Follower::receiveDiff(const std::vector<Commit>& diff)
{
  store_.applyDiff(diff);
  self_.send(leader_, AckNewLeader(self_.getId()));
}

void
Follower::receiveTruncate(const Trunc& trunc)
{
}

std::ostream&
operator<<(std::ostream& stream, const Follower::Info& info)
{
  stream << info.lastZxid_;
  return stream;
}
