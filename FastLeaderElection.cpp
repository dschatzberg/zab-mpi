#include "FastLeaderElection.hpp"

//const uint32_t TIMEOUT = 500;
//const uint32_t MAX_TIMEOUT = 60000;

FastLeaderElection::FastLeaderElection(QuorumPeer& self) : self_(self)
{
  epoch_ = 0;
}

void
FastLeaderElection::lookForLeader()
{
  vote_.from = self_.getId();
  vote_.leader = self_.getId();
  vote_.zxid = self_.getLastZxid();
  epoch_++;
  vote_.epoch = epoch_;
  vote_.state = State::Looking;
  //timeout_ = TimeoutDesc(TIMEOUT, handleTimeout);
  self_.broadcast(vote_);
}

void
FastLeaderElection::receiveVote(const Vote& v)
{
  if (self_.getState() == State::Looking) {
    if (v.state == State::Looking) {
      //handle_vote
      if (v.epoch < epoch_) {
        //this is an old vote, just ignore
        return;
      } else if (v.epoch > epoch_) {
        //we are behind on the election, update our epoch
        epoch_ = v.epoch;
        vote_.epoch = epoch_;
        updateVote(vote_, v); //if they have a better vote, update
        receivedVotes_.clear();
        self_.broadcast(vote_);
        addVote(v);
      } else {
        //same epoch
        //if they have a better vote, update ours and bcast
        if (updateVote(vote_, v)) {
          self_.broadcast(vote_);
        }
        addVote(v);
      }
    } else {
      //vote came from someone not electing

      if (v.epoch == epoch_) {
        receivedVotes_[v.from] = v;
        //do we have a quorum for this vote and
        // have they responded?
        if (haveQuorum(v, receivedVotes_) &&
            checkLeader(v.leader, receivedVotes_)) {
          vote_.leader = v.leader;
          vote_.zxid = v.zxid;
          self_.elected(vote_.leader, vote_.zxid);
          return;
        }
      }
      //is there an existing quorum with a leader?
      outOfElection_[v.from] = v;
      if (haveQuorum(v, outOfElection_) &&
          checkLeader(v.leader, outOfElection_)) {
        epoch_ = v.epoch;
        vote_.epoch = epoch_;
        vote_.leader = v.leader;
        vote_.zxid = v.zxid;
        self_.elected(vote_.leader, vote_.zxid);
      }
    }
  } else if (v.state == State::Looking) {
    //reply with our state
    vote_.state = self_.getState();
    self_.send(v.from, vote_);
  }
}

void
FastLeaderElection::addVote(const Vote& v)
{
  receivedVotes_[v.from] = v;
  if (haveQuorum(vote_, receivedVotes_)) {
    //should wait for a bit to hear if there is a change
    self_.elected(vote_.leader, vote_.zxid);
  }
}

bool
FastLeaderElection::updateVote(Vote& ours, const Vote& theirs) {
  if (theirs > ours) {
    ours.leader = theirs.leader;
    ours.zxid = theirs.zxid;
    return true;
  }
  return false;
}

bool
FastLeaderElection::haveQuorum(const Vote& v, const VoteMap& map) {
  unsigned int i = 1;
  for (VoteMap::const_iterator it = map.begin(); it != map.end(); it++) {
    if (v == it->second) {
      i++;
      if (i >= self_.getQuorumSize())
        return true;
    }
  }
  return false;
}

bool
FastLeaderElection::checkLeader(PeerId leader, const VoteMap& map) {
  //either everyone thinks we are the leader (in which case we are)
  // or we have a message from the leader
  return (leader == self_.getId()) || map.count(leader);
}
