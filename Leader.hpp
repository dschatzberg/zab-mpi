#ifndef LEADER_HPP
#define LEADER_HPP

#include <stdint.h>

#include <map>

#include "Message.pb.h"

class Log;
class QuorumPeer;

class Leader {
public:
  Leader(QuorumPeer& self, Log& store, const std::string& id);
  void Lead();
  uint64_t Propose(const std::string& message);
  void Receive(const Message::FollowerInfo& fi);
  void Receive(uint64_t zxid);
  void ReceiveAckNewLeader();
  // void receiveFollower(const Follower::Info& fi);
  // void receiveAckNewLeader(const AckNewLeader& anl);
  // void receiveAck(const ProposalAck& pa);
  // void write(uint32_t val);
private:
  QuorumPeer& self_;
  Log& log_;
  bool recovering_;
  bool leading_;
  uint32_t ack_count_;
  std::map<uint64_t, uint32_t> acks_;
  Message message_;
  Message propose_;
  Message commit_;
};

#endif
