#pragma once

#include "bc.hpp"

using namespace bc;

namespace fields {
class PotentialFieldSource {
 public:
}

class PotentialFunction {
 public:
  virtual float f(const MapLocation& loc) const = 0;
  virtual float grad(const MapLocation& loc) const = 0;
}

class PotentialField {
 public:
  PotentialField(PotentialFieldSource source) : source_(source) {}

  float evaluate(const PotentialFunction& potential,
                 const MapLocation& loc) const {}

 protected:
  PotentialFieldSource source_;
}
}  // namespace fields