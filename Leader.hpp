#ifndef LEADER_HPP
#define LEADER_HPP

#include <stdint.h>

#include <map>

#include "Zab.hpp"

class Log;
class QuorumPeer;

class Leader : public TimerManager::TimerCallback {
public:
  Leader(QuorumPeer& self, Log& store, TimerManager& tm,
         const std::string& id);
  virtual void Callback();
  void Lead();
  uint64_t Propose(const std::string& message);
  void Receive(const FollowerInfo& fi);
  void Receive(const AckNewLeader& anl);
  void Receive(const ProposalAck& pa);
private:
  void Ack(uint64_t zxid);
  QuorumPeer& self_;
  Log& log_;
  TimerManager& tm_;
  int recover_timer_;
  bool recovering_;
  bool leading_;
  uint32_t ack_count_;
  std::map<uint64_t, uint32_t> acks_;
  NewLeaderInfo nli_;
  Proposal p_;
  Commit c_;
};

#endif
