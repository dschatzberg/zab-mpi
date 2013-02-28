#ifndef FASTLEADERELECTION_HPP
#define FASTLEADERELECTION_HPP

#include <stdint.h>

#include <map>

#include "Message.pb.h"
#include "Zab.hpp"

class ReliableFifoCommunicator;
class QuorumPeer;
class Log;

class FastLeaderElection : public TimerManager::TimerCallback {
public:
  FastLeaderElection(QuorumPeer& self,
                     Log& log,
                     TimerManager& tm,
                     const std::string& id);
  void LookForLeader();
  void Receive(const Message::Vote& v);
  void Callback();
private:
  void SendVote();
  bool UpdateVote(const Message::Vote& v);
  void AddVote(const Message::Vote& v);
  void Elect(const std::string& leader, uint64_t zxid);

  QuorumPeer& self_;
  Log& log_;
  TimerManager& tm_;
  uint32_t epoch_;
  Message vote_;
  int timer_;
  bool waiting_;

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
};

#endif
