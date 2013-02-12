#ifndef LEADER_HPP
#define LEADER_HPP

#include <map>

#include "AckNewLeader.hpp"
#include "Follower.hpp"
#include "NewLeaderInfo.hpp"
#include "ProposalAck.hpp"
#include "Zxid.hpp"

class QuorumPeer;
class StableStore;

class Leader {
public:
  Leader(QuorumPeer& self, StableStore& store);
  void lead();
  void receiveFollower(const Follower::Info& fi);
  void receiveAckNewLeader(const AckNewLeader& anl);
  void receiveAck(const ProposalAck& pa);
  void write(uint32_t val);
private:
  QuorumPeer& self_;
  StableStore& store_;
  bool active_;
  uint32_t ackCount_;
  std::map<Zxid, uint32_t> acks_;
};

#endif
