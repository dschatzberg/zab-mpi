#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <string>
#include <vector>

#include "Message.pb.h"

class Log;
class QuorumPeer;

class Follower {
public:
  Follower(QuorumPeer& self, Log& log, const std::string& id);
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
  std::string leader_;
  bool recovering_;
  Message message_;
  Message ack_;
  Message ack_propose_;
};

#endif
