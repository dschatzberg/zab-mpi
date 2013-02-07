#ifndef FASTLEADERELECTION_HPP
#define FASTLEADERELECTION_HPP

#include <cstdint>

#include <map>

#include "PeerId.hpp"
#include "QuorumPeer.hpp"
#include "Vote.hpp"

class QuorumPeer;

class FastLeaderElection {
public:
  FastLeaderElection(QuorumPeer& self);
  void lookForLeader();
  void receiveVote(const Vote& v);
private:
  typedef std::map<PeerId, Vote> VoteMap;
  void addVote(const Vote& v);
  bool updateVote(Vote& ours, const Vote& theirs);
  bool haveQuorum(const Vote& v, const VoteMap& map);
  bool checkLeader(PeerId leader, const VoteMap& map);

  QuorumPeer& self_;
  Vote vote_;
  //FIXME: Use c++11 unordered_map
  VoteMap receivedVotes_;
  VoteMap outOfElection_;
  uint32_t epoch_;
  //TimeoutDesc timeout_;
};

#endif