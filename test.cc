#include <boost/thread.hpp>

#include "Zab.hpp"

class ZabCallbackImpl : public ZabCallback {
  virtual void Deliver(Zxid id, const std::string& message) {

  }
  virtual void Status(ZabStatus status, const std::string *leader) {

  }
};

class PipeCommunicator : public ReliableFifoCommunicator {
  virtual void Broadcast(const std::string& message) {
  }
  virtual void Send(const std::string& to, const std::string& message) {
  }
};

void
work(int index)
{
  ZabCallbackImpl cb;
  PipeCommunicator comm;
  std::stringstream ss;
  ss << index;
  Zab zab(cb,comm, ss.str());
  zab.Startup();
}

const int num_threads = 3;

int
main()
{
  boost::thread threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    threads[i] = boost::thread(work, i);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }
}
