#ifndef PEERID_HPP
#define PEERID_HPP

#include <cstdint>

#include <ostream>

class PeerId {
public:
  PeerId() : val_(0) {}
  PeerId(uintptr_t val) : val_(val) {}
  operator uintptr_t() const {
    return val_;
  }
  friend std::ostream& operator<<(std::ostream& stream, const PeerId& id);
private:
  uintptr_t val_;
};

#endif
