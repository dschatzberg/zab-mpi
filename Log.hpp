#ifndef LOG_HPP
#define LOG_HPP

#include <cstdint>

class Log {
public:
  Log();
  uint64_t LastZxid();
private:
  uint64_t last_zxid_;
};

#endif
