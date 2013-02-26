#include "FastLeaderElection.hpp"
#include "Log.hpp"
#include "Message.pb.h"
#include "QuorumPeer.hpp"
#include "Zab.hpp"

FastLeaderElection::FastLeaderElection(QuorumPeer& self,
                                       Log& log,
                                       const std::string& id)
  : self_(self), log_(log), epoch_(0)
{
  vote_.set_type(Message::VOTE);
  vote_.mutable_vote()->set_from(id);
  vote_.mutable_vote()->set_epoch(0);
}

void
FastLeaderElection::LookForLeader()
{
  vote_.mutable_vote()->set_leader(vote_.vote().from());
  vote_.mutable_vote()->set_zxid(log_.LastZxid());
  vote_.mutable_vote()->set_epoch(vote_.vote().epoch() + 1);
  vote_.mutable_vote()->set_status(Message::LOOKING);
  //should set a timeout
  SendVote();
  received_votes_.clear();
  out_of_election_.clear();
}

void
FastLeaderElection::Receive(const Message::Vote& v)
{
  if (vote_.vote().status() == Message::LOOKING) {
    if (v.status() == Message::LOOKING) {
      //handle vote
      if (v.epoch() < vote_.vote().epoch()) {
        //old vote, ignore it
        return;
      } else if (v.epoch() > vote_.vote().epoch()) {
        //we are behind on the election
        vote_.mutable_vote()->set_epoch(v.epoch());
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
      if (v.epoch() == vote_.vote().epoch()) {
        received_votes_[v.from()] = Vote(v);
        //do we have a quorum and have they responded?
        if (HaveQuorum(v, received_votes_) &&
            received_votes_.count(v.leader())) {
          vote_.mutable_vote()->set_leader(v.leader());
          vote_.mutable_vote()->set_zxid(v.zxid());
          Elect(vote_.vote().leader(), vote_.vote().zxid());
          return;
        }
      }

      //is there an existing quorum?
      out_of_election_[v.from()] = Vote(v);
      if (HaveQuorum(v, out_of_election_) &&
          out_of_election_.count(v.leader())) {
        vote_.mutable_vote()->set_leader(v.leader());
        vote_.mutable_vote()->set_zxid(v.zxid());
        Elect(vote_.vote().leader(), vote_.vote().zxid());
        return;
      }
    }
  } else if (v.status() == Message::LOOKING) {
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
FastLeaderElection::UpdateVote(const Message::Vote& v)
{
  if (v.zxid() > vote_.vote().zxid() || (v.zxid() == vote_.vote().zxid() &&
                                         v.leader() > vote_.vote().leader())) {
    vote_.mutable_vote()->set_leader(v.leader());
    vote_.mutable_vote()->set_zxid(v.zxid());
    return true;
  }
  return false;
}

void
FastLeaderElection::AddVote(const Message::Vote& v)
{
  received_votes_[v.from()] = Vote(v);
  if (HaveQuorum(Vote(vote_.vote()), received_votes_)) {
    //set a timeout to listen for more votes
    Elect(vote_.vote().leader(), vote_.vote().zxid());
  }
}

bool
FastLeaderElection::HaveQuorum(const Vote& v,
                               const std::map<std::string, Vote>& map)
{
  uint32_t i = 1;
  uint32_t size = self_.QuorumSize();
  for (std::map<std::string, Vote>::const_iterator it = map.begin();
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
  if (leader == vote_.vote().from()) {
    vote_.mutable_vote()->set_status(Message::LEADING);
  } else {
    vote_.mutable_vote()->set_status(Message::FOLLOWING);
  }
  self_.Elected(leader, zxid);
}
//const uint32_t TIMEOUT = 500;
//const uint32_t MAX_TIMEOUT = 60000;

// FastLeaderElection::FastLeaderElection(QuorumPeer& self) : self_(self)
// {
//   epoch_ = 0;
// }

// void
// FastLeaderElection::lookForLeader()
// {
//   vote_.from = self_.getId();
//   vote_.leader = self_.getId();
//   vote_.zxid = self_.getLastZxid();
//   epoch_++;
//   vote_.epoch = epoch_;
//   vote_.state = State::Looking;
//   //timeout_ = TimeoutDesc(TIMEOUT, handleTimeout);
//   self_.broadcast(vote_);
// }

// void
// FastLeaderElection::receiveVote(const Vote& v)
// {
//   if (self_.getState() == State::Looking) {
//     if (v.state == State::Looking) {
//       //handle_vote
//       if (v.epoch < epoch_) {
//         //this is an old vote, just ignore
//         return;
//       } else if (v.epoch > epoch_) {
//         //we are behind on the election, update our epoch
//         epoch_ = v.epoch;
//         vote_.epoch = epoch_;
//         updateVote(vote_, v); //if they have a better vote, update
//         receivedVotes_.clear();
//         self_.broadcast(vote_);
//         addVote(v);
//       } else {
//         //same epoch
//         //if they have a better vote, update ours and bcast
//         if (updateVote(vote_, v)) {
//           self_.broadcast(vote_);
//         }
//         addVote(v);
//       }
//     } else {
//       //vote came from someone not electing

//       if (v.epoch == epoch_) {
//         receivedVotes_[v.from] = v;
//         //do we have a quorum for this vote and
//         // have they responded?
//         if (haveQuorum(v, receivedVotes_) &&
//             checkLeader(v.leader, receivedVotes_)) {
//           vote_.leader = v.leader;
//           vote_.zxid = v.zxid;
//           self_.elected(vote_.leader, vote_.zxid);
//           return;
//         }
//       }
//       //is there an existing quorum with a leader?
//       outOfElection_[v.from] = v;
//       if (haveQuorum(v, outOfElection_) &&
//           checkLeader(v.leader, outOfElection_)) {
//         epoch_ = v.epoch;
//         vote_.epoch = epoch_;
//         vote_.leader = v.leader;
//         vote_.zxid = v.zxid;
//         self_.elected(vote_.leader, vote_.zxid);
//       }
//     }
//   } else if (v.state == State::Looking) {
//     //reply with our state
//     vote_.state = self_.getState();
//     self_.send(v.from, vote_);
//   }
// }

// void
// FastLeaderElection::addVote(const Vote& v)
// {
//   receivedVotes_[v.from] = v;
//   if (haveQuorum(vote_, receivedVotes_)) {
//     //should wait for a bit to hear if there is a change
//     self_.elected(vote_.leader, vote_.zxid);
//   }
// }

// bool
// FastLeaderElection::updateVote(Vote& ours, const Vote& theirs) {
//   if (theirs > ours) {
//     ours.leader = theirs.leader;
//     ours.zxid = theirs.zxid;
//     return true;
//   }
//   return false;
// }

// bool
// FastLeaderElection::haveQuorum(const Vote& v, const VoteMap& map) {
//   unsigned int i = 1;
//   unsigned int size = self_.getQuorumSize();
//   for (VoteMap::const_iterator it = map.begin(); it != map.end(); it++) {
//     if (v == it->second) {
//       i++;
//       if (i >= size)
//         return true;
//     }
//   }
//   return false;
// }

// bool
// FastLeaderElection::checkLeader(PeerId leader, const VoteMap& map) {
//   //either everyone thinks we are the leader (in which case we are)
//   // or we have a message from the leader
//   return (leader == self_.getId()) || map.count(leader);
// }
