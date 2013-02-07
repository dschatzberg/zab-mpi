#ifndef ZXID_HPP
#define ZXID_HPP

#include <cstdint>

#include <ostream>
#include <utility>

class Zxid {
public:
  Zxid() : epoch_(0), count_(0) {}
  inline void
  newEpoch()
  {
    epoch_++;
    count_ = 0;
  }
  bool operator<(const Zxid& rhs) const {
    return (epoch_ < rhs.epoch_) ||
      (epoch_ == rhs.epoch_ && count_ < rhs.count_);
  }
  bool operator>(const Zxid& rhs) const {
    return rhs < *this;
  }
  bool operator==(const Zxid& rhs) const {
    return epoch_ == rhs.epoch_ && count_ == rhs.count_;
  }
  bool operator<=(const Zxid& rhs) const {
    return !(*this > rhs);
  }
  bool operator>=(const Zxid& rhs) const {
    return !(*this < rhs);
  }
  friend std::ostream& operator<<(std::ostream& stream, const Zxid& id);
private:
  uint32_t epoch_;
  uint32_t count_;
};
//typedef std::pair<uint32_t, uint32_t> Zxid;

#endif
