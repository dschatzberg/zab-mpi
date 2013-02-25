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

class TimerManager {
public:
  class TimerCallback {
  public:
    virtual void Callback() = 0;
    virtual ~TimerCallback() {}
  };
  virtual int Alloc(TimerCallback* cb) = 0;
  virtual void Free(int timer) = 0;
  virtual void Arm(int timer, uint64_t cycles) = 0;
  virtual void Disarm(int timer) = 0;
  virtual ~TimerManager() {}
};

class Zab {
public:
  virtual uint64_t Propose(const std::string& message) = 0;

  virtual void Startup() = 0;

  virtual void Receive(const std::string& message) = 0;

  virtual ~Zab() {}
};

#endif
