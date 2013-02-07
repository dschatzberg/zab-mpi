#ifndef FOLLOWER_HPP
#define FOLLOWER_HPP

#include <vector>

#include "Commit.hpp"
#include "NewLeaderInfo.hpp"
#include "PeerId.hpp"
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
};

#endif
