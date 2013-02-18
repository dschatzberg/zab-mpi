#include <iostream>

#include "ZabImpl.hpp"

class ZabCallbackImpl : public ZabCallback {
public:
  virtual void Deliver(uint64_t id, const std::string& message)
  {
    std::cout << "Delivered \"" << message <<
      "\" with zxid 0x" << std::hex << id << std::endl;
  }
  virtual void Status(ZabStatus status, const std::string *leader)
  {
    std::cout << "Status Changed" << std::endl;
  }
};

class BGCommunicator : public ReliableFifoCommunicator {
public:
  virtual void Broadcast(const std::string& message)
  {
  }
  virtual void Send(const std::string& to, const std::string& message)
  {
  }
  virtual uint32_t Size()
  {
    return 3;
  }
};

int main()
{
  ZabCallbackImpl cb;
  BGCommunicator comm;
  ZabImpl zab(cb, comm, "0");
  zab.Startup();
}
