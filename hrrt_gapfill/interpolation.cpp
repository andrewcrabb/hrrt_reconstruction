#include <iostream>
#include "interpolation.h"
using namespace std;

// Linear interpolation:
//   f(x) = f(x0) + ((f(x1) - f(x0))*(x - x0))/(x1 - x0)
float linearinterpolate(float x, float x0, float x1, float f0, float f1)
{
  return f0 + ((f1 - f0)*(x - x0))/(x1 - x0);
}
