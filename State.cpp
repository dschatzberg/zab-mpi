#include "State.hpp"

std::ostream& operator<<(std::ostream &stream, const State& state) {
  switch (state) {
  case Looking:
    stream << "Looking";
    break;
  case Following:
    stream << "Following";
    break;
  case Leading:
    stream << "Leading";
    break;
  }
  return stream;
}
