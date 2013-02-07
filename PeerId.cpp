#include "PeerId.hpp"

std::ostream&
operator<<(std::ostream& stream, const PeerId& id) {
  stream << id.val_;
  return stream;
}
