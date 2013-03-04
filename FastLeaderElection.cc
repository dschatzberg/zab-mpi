#include "FastLeaderElection.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"
#include "Zab.hpp"

namespace {
  const uint64_t ELECT_TIMEOUT = 425000000;
}

FastLeaderElection::FastLeaderElection(QuorumPeer& self,
                                       Log& log,
                                       TimerManager& tm,
                                       const std::string& id)
  : self_(self), log_(log), tm_(tm), epoch_(0)
{
  timer_ = tm_.Alloc(this);
  vote_.set_from(id);
  vote_.set_epoch(0);
}

void
FastLeaderElection::LookForLeader()
{
  vote_.set_leader(vote_.from());
  vote_.set_zxid(log_.LastZxid());
  vote_.set_epoch(vote_.epoch() + 1);
  vote_.set_status(Vote::LOOKING);
  //should set a timeout
  SendVote();
  received_votes_.clear();
  out_of_election_.clear();
  waiting_ = false;
}

void
FastLeaderElection::Receive(const Vote& v)
{
  if (vote_.status() == Vote::LOOKING) {
    if (v.status() == Vote::LOOKING) {
      //handle vote
      if (v.epoch() < vote_.epoch()) {
        //old vote, ignore it
        return;
      } else if (v.epoch() > vote_.epoch()) {
        //we are behind on the election
        vote_.set_epoch(v.epoch());
        UpdateVote(v);
        received_votes_.clear();
        SendVote();
        AddVote(v);
      } else {
        //same epoch
        //if they have a better vote, update ours and send
        if (UpdateVote(v)) {
          SendVote();
        }
        AddVote(v);
      }
    } else {
      //vote came from someone not electing
      if (v.epoch() == vote_.epoch()) {
        received_votes_[v.from()] = Vote(v);
        //do we have a quorum and have they responded?
        if (HaveQuorum(v, received_votes_) &&
            received_votes_.count(v.leader())) {
          vote_.set_leader(v.leader());
          vote_.set_zxid(v.zxid());
          Elect(vote_.leader(), vote_.zxid());
          return;
        }
      }

      //is there an existing quorum?
      out_of_election_[v.from()] = Vote(v);
      if (HaveQuorum(v, out_of_election_) &&
          out_of_election_.count(v.leader())) {
        vote_.set_leader(v.leader());
        vote_.set_zxid(v.zxid());
        Elect(vote_.leader(), vote_.zxid());
        return;
      }
    }
  } else if (v.status() == Vote::LOOKING) {
    //reply with our state
    self_.Send(v.from(), vote_);
  }
}

void
FastLeaderElection::SendVote()
{
  self_.Broadcast(vote_);
}

bool
FastLeaderElection::UpdateVote(const Vote& v)
{
  if (v.zxid() > vote_.zxid() || (v.zxid() == vote_.zxid() &&
                                         v.leader() > vote_.leader())) {
    vote_.set_leader(v.leader());
    vote_.set_zxid(v.zxid());
    if (waiting_) {
      waiting_ = false;
      tm_.Disarm(timer_);
    }
    return true;
  }
  return false;
}

void
FastLeaderElection::AddVote(const Vote& v)
{
  received_votes_[v.from()] = Vote(v);
  if (HaveQuorum(Vote(vote_), received_votes_)) {
    //set a timeout to listen for more votes
    if (!waiting_) {
      waiting_ = true;
      tm_.Arm(timer_, ELECT_TIMEOUT);
    }
  }
}

void
FastLeaderElection::Callback() {
  Elect(vote_.leader(), vote_.zxid());
}

bool
FastLeaderElection::HaveQuorum(const VoteWrapper& v,
                               const std::map<std::string, VoteWrapper>& map)
{
  uint32_t i = 1;
  uint32_t size = self_.QuorumSize();
  for (std::map<std::string, VoteWrapper>::const_iterator it = map.begin();
       it != map.end();
       ++it) {
    if (v == it->second) {
      ++i;
      if (i >= size)
        return true;
    }
  }
  return false;
}

void
FastLeaderElection::Elect(const std::string& leader, uint64_t zxid)
{
  if (leader == vote_.from()) {
    vote_.set_status(Vote::LEADING);
  } else {
    vote_.set_status(Vote::FOLLOWING);
  }
  self_.Elected(leader, zxid);
}
