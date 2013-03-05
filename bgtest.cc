#include <iomanip>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <utility>

#include <bpcore/bgp_collective_inlines.h>
#include <common/bgp_personality_inlines.h>
#include <spi/DMA_Addressing.h>
#include <spi/GlobInt.h>

#include "BGCommunicator.hpp"
#include "Zab.hpp"

namespace {
  const uint64_t NUM_WRITES = 1000;
}
class ZabCallbackImpl : public ZabCallback {
public:
  ZabCallbackImpl(Zab*& zab, TimerManager& tm, const std::string& id) :
    zab_(zab), tm_(tm), cb_(*this), id_(id),
    count_(0), old_mean_(0), old_s_(0), new_mean_(0), new_s_(0),
    max_(0)
  {
    timer_ = tm_.Alloc(&cb_);
  }
  virtual void Deliver(uint64_t id, const std::string& message)
  {
    count_++;
    if (status_ == LEADING) {
      uint64_t val = _bgp_GetTimeBase() - time_;
      max_ = std::max(max_, val);
      if (count_ == 1) {
        old_mean_ = new_mean_ = val;
        old_s_ = 0.0;
      } else {
        new_mean_ = old_mean_ + (val - old_mean_)/count_;
        new_s_ = old_s_ + (val - old_mean_)*(val - new_mean_);

        old_mean_ = new_mean_;
        old_s_ = new_s_;
      }
      if (count_ < NUM_WRITES) {
        tm_.Arm(timer_, 10000000);
      } else {
        std::cout << "Latency = " << new_mean_ << ", variance = " <<
          new_s_/(count_ - 1) << ", max = " << max_ << std::endl;
      }
    }
    if (count_ >= NUM_WRITES) {
      GlobInt_Barrier(0, NULL, NULL);
      exit(0);
    }
  }
  virtual void Status(ZabStatus status, const std::string *leader)
  {
    status_ = status;
    if (status != LOOKING) {
      std::cout << id_ << ": Elected " << *leader << std::endl;
    }
    if (status == LEADING) {
      tm_.Arm(timer_, 8500000000ULL);
    }
  }
private:
  void Test()
  {
    time_ = _bgp_GetTimeBase();
    zab_->Propose("test");
  }
  class MyTimerCallback : public TimerManager::TimerCallback {
  public:
    MyTimerCallback(ZabCallbackImpl& self) : self_(self) {}
    virtual void Callback() {
      self_.Test();
    }
  private:
    ZabCallbackImpl& self_;
  };
  ZabStatus status_;
  Zab*& zab_;
  TimerManager& tm_;
  MyTimerCallback cb_;
  int timer_;
  const std::string id_;
  uint64_t time_;
  uint64_t count_;
  double old_mean_;
  double old_s_;
  double new_mean_;
  double new_s_;
  uint64_t max_;
};

class TimerManagerImpl : public TimerManager {
public:
  TimerManagerImpl() : count_(0)
  {
  }
  virtual int Alloc(TimerManager::TimerCallback *cb)
  {
    int ret;
    if (!free_list_.empty()) {
      ret = free_list_.top();
      free_list_.pop();
    } else {
      ret = count_;
      count_++;
    }
    cb_map_[ret] = cb;
    return ret;
  }
  virtual void Free(int timer)
  {
    free_list_.push(timer);
  }

  virtual void Arm(int timer, uint64_t cycles)
  {
    uint64_t time = _bgp_GetTimeBase() + cycles;
    std::list<std::pair<uint64_t, int> >::iterator it;
    for (it = timer_list_.begin();
         it != timer_list_.end();
         it++) {
      if (it->first > time) {
        break;
      }
    }
    timer_list_.insert(it, std::make_pair(time, timer));
  }

  virtual void Disarm(int timer)
  {
    for (std::list<std::pair<uint64_t, int> >::iterator
           it = timer_list_.begin();
         it != timer_list_.end();
         it++) {
      if (it->second == timer) {
        timer_list_.erase(it);
        return;
      }
    }
  }

  void Dispatch()
  {
    while (!timer_list_.empty()) {
      if (timer_list_.front().first <= _bgp_GetTimeBase()) {
        cb_map_[timer_list_.front().second]->Callback();
        timer_list_.pop_front();
      } else {
        return;
      }
    }
  }
private:
  int count_;
  std::stack<int> free_list_;
  std::map<int, TimerManager::TimerCallback*> cb_map_;
  std::list<std::pair<uint64_t, int> > timer_list_;
};

int main()
{
  Zab* zab;
  BGCommunicator comm(zab);
  GlobInt_Barrier(0, NULL, NULL);
  _BGP_Personality_t pers;
  Kernel_GetPersonality(&pers, sizeof(pers));
  std::stringstream ss;
  ss << pers.Network_Config.Rank;
  TimerManagerImpl tm;
  ZabCallbackImpl cb(zab, tm, ss.str());
  zab = new Zab(cb, comm, tm, ss.str());
  zab->Startup();
  while (1) {
    comm.Dispatch();
    tm.Dispatch();
  }
  delete zab;
  return 0;
}
