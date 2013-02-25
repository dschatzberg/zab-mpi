#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <string>
#include <vector>

#include "Message.pb.h"
#include "Zab.hpp"

class Log;
class QuorumPeer;

class Follower : public TimerManager::TimerCallback {
public:
  Follower(QuorumPeer& self, Log& log, TimerManager& tm, const std::string& id);
  virtual void Callback();
  void Recover(const std::string& leader);
  void Receive(const Message::NewLeaderInfo& nli);
  void Receive(const Message::Entry& proposal);
  void Receive(uint64_t zxid);
  // void receiveLeader(const NewLeaderInfo& info);
  // void receiveDiff(const std::vector<Commit>& diff);
  // void receiveTruncate(const Trunc& trunc);
  // void receiveProposal(const Proposal& p);
  // void receiveCommit(const CommitMsg& cm);
  // class Info {
  // public:
  //   Info() {}
  //   Info(PeerId from, Zxid zxid) : from_(from), lastZxid_(zxid) {}
  //   friend std::ostream& operator<<(std::ostream& stream, const Info& info);

  //   PeerId from_;
  //   Zxid lastZxid_;
  // };
private:
  QuorumPeer& self_;
  Log& log_;
  TimerManager& tm_;
  int recover_timer_;
  std::string leader_;
  bool following_;
  bool recovering_;
  Message message_;
  Message ack_;
  Message ack_propose_;
};

#endif
