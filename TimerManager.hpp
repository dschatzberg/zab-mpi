#ifndef TIMERMANAGER_HPP
#define TIMERMANAGER_HPP

#include <stdint.h>

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

#endif
