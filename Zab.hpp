#ifndef ZAB_HPP
#define ZAB_HPP

#include "FastLeaderElection.hpp"
#include "Log.hpp"
#include "Zxid.hpp"

enum ZabStatus {
  LOOKING,
  FOLLOWING,
  LEADING
};

class ZabCallback {
public:
  virtual void Deliver(Zxid id, const std::string& message) = 0;
  virtual void Status(ZabStatus status, const std::string *leader) = 0;
};

class ReliableFifoCommunicator {
public:
  virtual void Broadcast(const std::string& message) = 0;
  virtual void Send(const std::string& to, const std::string& message) = 0;
};

class Zab {
public:
  Zab(ZabCallback& cb, ReliableFifoCommunicator& comm, const std::string& id);

  Zxid Propose(const std::string& message);

  void Startup();

  void Receive(const std::string& message);
private:
  void LookForLeader();
  ZabCallback& cb_;
  ReliableFifoCommunicator& comm_;
  const std::string& id_;
  ZabStatus status_;
  Log log_;
  FastLeaderElection fle_;
};

#endif
