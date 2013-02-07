#include "Vote.hpp"

std::ostream&
operator<<(std::ostream& stream, const Vote& id) {
  stream << "from: " << id.from << ", leader: " << id.leader <<
    ", zxid: " << id.zxid << ", epoch: " << id.epoch <<
    ", state: " << id.state;
  return stream;
}
