#ifndef FASTLEADERELECTION_HPP
#define FASTLEADERELECTION_HPP

#include <cstdint>

#include "Message.pb.h"

class ReliableFifoCommunicator;
class Log;

class FastLeaderElection {
public:
  FastLeaderElection(ReliableFifoCommunicator& comm, Log& log,
                     const std::string& id);
  void LookForLeader();
private:
  void SendVote();

  ReliableFifoCommunicator& comm_;
  Log& log_;
  const std::string& id_;
  uint32_t epoch_;
  Message::Vote vote_;
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
