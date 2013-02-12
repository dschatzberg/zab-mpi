#include "QuorumPeer.hpp"
#include "FastLeaderElection.hpp"

class QuorumPeerLinux : public QuorumPeer {
public:
  QuorumPeerTest(uintptr_t id, );
  PeerId getId();
  Zxid getLastZxid();
  void elected(PeerId leader, Zxid zxid);
  size_t getQuorumSize();
  State getState();
  void broadcast(const Vote& v);
  void send(PeerId destination, const Vote& v);
  void receiveVote(const Vote& v);
private:
  FastLeaderElection fle_;
  PeerId peerId_;
  Zxid zxid_;
  size_t qsize_;
  State state_;
};
