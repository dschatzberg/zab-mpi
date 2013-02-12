#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <vector>

#include "Commit.hpp"
#include "CommitMsg.hpp"
#include "NewLeaderInfo.hpp"
#include "PeerId.hpp"
#include "Proposal.hpp"
#include "Trunc.hpp"
#include "Zxid.hpp"

class QuorumPeer;
class StableStore;

class Follower {
public:
  Follower(QuorumPeer& self, StableStore& store);
  void recover(PeerId leader);
  void receiveLeader(const NewLeaderInfo& info);
  void receiveDiff(const std::vector<Commit>& diff);
  void receiveTruncate(const Trunc& trunc);
  void receiveProposal(const Proposal& p);
  void receiveCommit(const CommitMsg& cm);
  class Info {
  public:
    Info() {}
    Info(PeerId from, Zxid zxid) : from_(from), lastZxid_(zxid) {}
    friend std::ostream& operator<<(std::ostream& stream, const Info& info);

    PeerId from_;
    Zxid lastZxid_;
  };
private:
  QuorumPeer& self_;
  StableStore& store_;
  PeerId leader_;
  bool active;
};

#endif
