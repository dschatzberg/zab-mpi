#ifndef ZAB_HPP
#define ZAB_HPP

#include <stdint.h>

#include <string>

enum ZabStatus {
  LOOKING,
  FOLLOWING,
  LEADING
};

class ZabCallback {
public:
  virtual void Deliver(uint64_t id, const std::string& message) = 0;
  virtual void Status(ZabStatus status, const std::string *leader) = 0;
  virtual ~ZabCallback() {};
};

class ReliableFifoCommunicator {
public:
  virtual void Broadcast(const std::string& message) = 0;
  virtual void Send(const std::string& to, const std::string& message) = 0;
  virtual uint32_t Size() = 0;
  virtual ~ReliableFifoCommunicator() {};
};

class Zab {
public:
  virtual uint64_t Propose(const std::string& message) = 0;

  virtual void Startup() = 0;

  virtual void Receive(const std::string& message) = 0;

  virtual ~Zab() {}
};

#endif
