#ifndef LEADER_HPP
#define LEADER_HPP

#include "AckNewLeader.hpp"
#include "Follower.hpp"
#include "NewLeaderInfo.hpp"
#include "Zxid.hpp"

class QuorumPeer;
class StableStore;

class Leader {
public:
  Leader(QuorumPeer& self, StableStore& store);
  void lead();
  void receiveFollower(const Follower::Info& fi);
  void receiveAckNewLeader(const AckNewLeader& anl);
private:
  QuorumPeer& self_;
  StableStore& store_;
};

#endif
