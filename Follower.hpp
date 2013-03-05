#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <map>
#include <string>
#include <vector>

#include "Commit.pb.h"
#include "AckNewLeader.pb.h"
#include "FollowerInfo.pb.h"
#include "NewLeaderInfo.pb.h"
#include "Proposal.pb.h"
#include "ProposalAck.pb.h"
#include "TimerManager.hpp"

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
  void Receive(uint32_t count, uint64_t zxid);
private:
  QuorumPeer& self_;
  Log& log_;
  TimerManager& tm_;
  const std::string& id_;
  int recover_timer_;
  std::string leader_;
  bool following_;
  bool recovering_;
  FollowerInfo fi_;
  AckNewLeader anl_;
  ProposalAck pa_;
};

#endif
