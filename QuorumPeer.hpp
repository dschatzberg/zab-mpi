#ifndef QUORUMPEER_HPP
#define QUORUMPEER_HPP

#include <cstdint>

class QuorumPeer {
public:
  virtual void Send(const std::string& dest, const Message& message) = 0;
  virtual void Broadcast(const Message& message) = 0;
  virtual void Elected(const std::string& leader, uint64_t zxid) = 0;
  virtual void Ready() = 0;
  virtual int QuorumSize() = 0;
};

#endif
