#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "ZabImpl.hpp"

const int num_threads = 3;
int pipefd[num_threads][2];
uint32_t read_size[num_threads];

class ZabCallbackImpl : public ZabCallback {
public:
  ZabCallbackImpl(int id) : id_(id) {}
  virtual void Deliver(uint64_t id, const std::string& message) {
    std::cout << id_ << ": Delivered \"" << message <<
      "\" with zxid 0x" << std::hex << id << std::endl;
  }
  virtual void Status(ZabStatus status, const std::string *leader) {
    if (status == LEADING) {
      zab_->Propose("Test1");
      zab_->Propose("Test2");
      zab_->Propose("Test3");
    }
  }
  void set_zab(Zab* zab)
  {
    zab_ = zab;
  }
private:
  int id_;
  Zab* zab_;
};

class PipeCommunicator : public ReliableFifoCommunicator {
public:
  PipeCommunicator(int id) : id_(id) {}
  virtual void Broadcast(const std::string& message)
  {
    for (int i = 0; i < num_threads; i++) {
      if (i != id_) {
        uint32_t size = message.size();
        std::string to_send = std::string(reinterpret_cast<char*>(&size),
                                          sizeof(uint32_t)) + message;
        ssize_t ret = write(pipefd[i][1], to_send.data(), to_send.size());
        if (static_cast<size_t>(ret) != to_send.size()) {
          std::cerr << "write failed!" << std::endl;
          throw;
        }
      }
    }
  }
  virtual void Send(const std::string& to, const std::string& message)
  {
    std::stringstream convert(to);
    int id;
    convert >> id;
    uint32_t size = message.size();
    std::string to_send = std::string(reinterpret_cast<char*>(&size),
                                      sizeof(uint32_t)) + message;
    ssize_t ret = write(pipefd[id][1], to_send.data(), to_send.size());
    if (static_cast<size_t>(ret) != to_send.size()) {
      std::cerr << "write failed!" << std::endl;
      throw;
    }
  }

  virtual uint32_t Size() {
    return num_threads;
  }
private:
  int id_;
};

class EventLoop {
public:
  void
  HandleReceive(const boost::system::error_code& error)
  {
    if (!error) {
      char *str = static_cast<char*>(malloc(message_size));
      boost::asio::read(in_, boost::asio::buffer(str, message_size));
      z_.Receive(std::string(str, message_size));
      free(str);
      char* read_addr = reinterpret_cast<char*>(&message_size);
      boost::asio::async_read(in_,
                              boost::asio::buffer(read_addr, sizeof(uint32_t)),
                              boost::bind(&EventLoop::HandleReceive,
                                          this,
                                          boost::asio::placeholders::error));
    }
  }

  EventLoop(int id, Zab& z) : id_(id), io_(), in_(io_, pipefd[id][0]), z_(z)
  {
    char* read_addr = reinterpret_cast<char*>(&message_size);
    boost::asio::async_read(in_,
                            boost::asio::buffer(read_addr, sizeof(uint32_t)),
                            boost::bind(&EventLoop::HandleReceive,
                                        this,
                                        boost::asio::placeholders::error));
  }

  void
  Run() {
    io_.run();
  }
private:
  int id_;
  boost::asio::io_service io_;
  boost::asio::posix::stream_descriptor in_;
  uint32_t message_size;
  Zab& z_;
};

void
work(int index, boost::barrier& bar)
{
  bar.wait();

  ZabCallbackImpl cb(index);
  PipeCommunicator comm(index);
  std::stringstream ss;
  ss << index;
  ZabImpl zab(cb, comm, ss.str());
  cb.set_zab(&zab);
  zab.Startup();
  EventLoop loop(index, zab);
  loop.Run();
}


int
main()
{
  for (int i = 0; i < num_threads; i++) {
    pipe(pipefd[i]);
  }

  boost::barrier bar(num_threads);
  boost::thread_group threads;
  for (int i = 0; i < num_threads; i++) {
    threads.create_thread(boost::bind(&work, i, boost::ref(bar)));
  }

  threads.join_all();

  return 0;
}
