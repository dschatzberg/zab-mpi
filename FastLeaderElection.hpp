#ifndef FASTLEADERELECTION_HPP
#define FASTLEADERELECTION_HPP

#include <cstdint>

#include <map>

#include "Message.pb.h"

class ReliableFifoCommunicator;
class QuorumPeer;
class Log;

class FastLeaderElection {
public:
  FastLeaderElection(QuorumPeer& self,
                     Log& log, const std::string& id);
  void LookForLeader();
  void Receive(const Message::Vote& v);
private:
  void SendVote();
  bool UpdateVote(const Message::Vote& v);
  void AddVote(const Message::Vote& v);
  void Elect(const std::string& leader, uint64_t zxid);

  QuorumPeer& self_;
  Log& log_;
  uint32_t epoch_;
  Message vote_;

  class Vote {
  public:
    Vote() {}
    Vote(const Message::Vote &v) : leader_(v.leader()), zxid_(v.zxid()) {}
    inline bool operator<(const Vote& rhs) const {
      return zxid_ < rhs.zxid_ || (zxid_ == rhs.zxid_ && leader_ < rhs.leader_);
    }
    inline bool operator>(const Vote& rhs) const {
      return rhs < *this;
    }
    inline bool operator==(const Vote& rhs) const {
      return zxid_ == rhs.zxid_ && leader_ == rhs.leader_;
    }
  private:
    std::string leader_;
    uint64_t zxid_;
  };
  bool HaveQuorum(const Vote& v, const std::map<std::string, Vote>& map);


  std::map<std::string, Vote> received_votes_;
  std::map<std::string, Vote> out_of_election_;
//   void Receive(const Vote& v);
// private:
//   typedef std::map<PeerId, Vote> VoteMap;
//   void addVote(const Vote& v);
//   bool updateVote(Vote& ours, const Vote& theirs);
//   bool haveQuorum(const Vote& v, const VoteMap& map);
//   bool checkLeader(PeerId leader, const VoteMap& map);

//   QuorumPeer& self_;
//   Vote vote_;
//   //FIXME: Use c++11 unordered_map
//   VoteMap receivedVotes_;
//   VoteMap outOfElection_;
//   uint32_t epoch_;
//   //TimeoutDesc timeout_;
};

#endif
