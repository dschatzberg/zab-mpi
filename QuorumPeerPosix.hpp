#ifndef QUORUMPEERPOSIX_HPP
#define QUORUMPEERPOSIX_HPP

#include <boost/asio.hpp>

#include "FastLeaderElection.hpp"
#include "Message.hpp"
#include "QuorumPeer.hpp"
#include "StableStore.hpp"

class QuorumPeerPosix : QuorumPeer {
public:
  QuorumPeerPosix(PeerId id,
                  StableStore& store,
                  const std::map<PeerId, int>& comm,
                  int in);
  void run();
  PeerId getId();
  Zxid getLastZxid();
  void newEpoch();
  void elected(PeerId leader, Zxid zxid);
  size_t getQuorumSize();
  State getState();
  void broadcast(const Vote& v);
  void send(PeerId destination, const Vote& v);
  void send(PeerId destination, const Follower::Info& fi);
  void send(PeerId destination, const NewLeaderInfo& nli);
  void send(PeerId destination, const std::vector<Commit>& diff);
  void send(PeerId destination, const Trunc& trunc);
  void send(PeerId destination, const AckNewLeader& anl);
private:
  void handleReceive(const boost::system::error_code& error);
  const PeerId id_;
  StableStore store_;
  State state_;
  boost::asio::io_service io_;
  boost::asio::posix::stream_descriptor in_;
  std::map<PeerId, int> comm_;
  FastLeaderElection fle_;
  Follower follower_;
  Leader leader_;
  Message message_;
  Zxid zxid_;
};

#endif
