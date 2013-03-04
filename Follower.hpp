#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <string>
#include <vector>

#include "Zab.hpp"

class Log;
class QuorumPeer;

class Follower : public TimerManager::TimerCallback {
public:
  Follower(QuorumPeer& self, Log& log, TimerManager& tm, const std::string& id);
  virtual void Callback();
  void Recover(const std::string& leader);
  void Receive(const NewLeaderInfo& nli);
  void Receive(const Proposal& p);
  void Receive(const Commit& c);
private:
  QuorumPeer& self_;
  Log& log_;
  TimerManager& tm_;
  int recover_timer_;
  std::string leader_;
  bool following_;
  bool recovering_;
  FollowerInfo fi_;
  AckNewLeader anl_;
  ProposalAck pa_;
};

#endif
