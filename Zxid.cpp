#include "Zxid.hpp"

std::ostream&
operator<<(std::ostream& stream, const Zxid& id) {
  stream << "(" << id.epoch_ << ", " << id.count_ << ")";
  return stream;
}
