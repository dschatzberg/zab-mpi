#include "Follower.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"

Follower::Follower(QuorumPeer& self, Log& log, const std::string& id) :
  self_(self), log_(log), recovering_(true)
{
  message_.set_type(Message::FOLLOWERINFO);
  message_.mutable_follower_info()->set_from(id);
  ack_.set_type(Message::ACKNEWLEADER);
  ack_propose_.set_type(Message::ACKPROPOSAL);
}

void
Follower::Recover(const std::string& leader)
{
  //FIXME: setup a timeout
  leader_ = leader;
  message_.mutable_follower_info()->set_zxid(log_.LastZxid());
  self_.Send(leader, message_);
}

void
Follower::Receive(const Message::NewLeaderInfo& nli)
{
  switch(nli.type()) {
  case Message::NewLeaderInfo::DIFF:
    for (int i = 0; i < nli.diff_size(); i++) {
      log_.Accept(nli.diff(i).zxid(), nli.diff(i).message());
    }
    for (int i = 0; i < nli.diff_size(); i++) {
      log_.Commit(nli.diff(i).zxid());
    }
    break;
  }
  self_.Send(leader_, ack_);
  recovering_ = false;
  self_.Ready();
}

void
Follower::Receive(const Message::Entry& proposal)
{
  log_.Accept(proposal.zxid(), proposal.message());
  ack_propose_.set_ack_zxid(proposal.zxid());
  self_.Send(leader_, ack_propose_);
}

void
Follower::Receive(uint64_t zxid)
{
  log_.Commit(zxid);
}

// void
// Follower::receiveLeader(const NewLeaderInfo& info)
// {
// }

// void
// Follower::receiveDiff(const std::vector<Commit>& diff)
// {
//   store_.applyDiff(diff);
//   self_.send(leader_, AckNewLeader(self_.getId()));
//   active = true;
//   self_.ready();
// }

// void
// Follower::receiveTruncate(const Trunc& trunc)
// {
// }

// void
// Follower::receiveProposal(const Proposal& p)
// {
//   store_.add_proposal(p);
//   ProposalAck ack(self_.getId(), p.zxid());
//   self_.send(p.from(), ack);
// }

// void
// Follower::receiveCommit(const CommitMsg& cm)
// {
//   store_.commit(cm.zxid());
// }

// std::ostream&
// operator<<(std::ostream& stream, const Follower::Info& info)
// {
//   stream << info.lastZxid_;
//   return stream;
// }
