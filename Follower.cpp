#include "Follower.hpp"
#include "PeerId.hpp"
#include "QuorumPeer.hpp"
#include "StableStore.hpp"

Follower::Follower(QuorumPeer& self, StableStore& store) :
  self_(self), store_(store), active(false)
{
}

void
Follower::recover(PeerId leader)
{
  //FIXME: setup a timeout
  active = false;
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
  active = true;
  self_.ready();
}

void
Follower::receiveTruncate(const Trunc& trunc)
{
}

void
Follower::receiveProposal(const Proposal& p)
{
  store_.add_proposal(p);
  ProposalAck ack(self_.getId(), p.zxid());
  self_.send(p.from(), ack);
}

void
Follower::receiveCommit(const CommitMsg& cm)
{
  store_.commit(cm.zxid());
}

std::ostream&
operator<<(std::ostream& stream, const Follower::Info& info)
{
  stream << info.lastZxid_;
  return stream;
}
