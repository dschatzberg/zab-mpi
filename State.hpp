#ifndef STATE_HPP
#define STATE_HPP

#include <ostream>

enum State {
  Looking,
  Following,
  Leading
};

std::ostream& operator<<(std::ostream &stream, const State& state);

#endif
