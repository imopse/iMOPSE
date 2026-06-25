#pragma once
#include <cmath>
inline double pdiv(double a, double b) { return std::fabs(b) < 1e-9 ? a : a / b; }
inline double pmin(double a, double b) { return a < b ? a : b; }
inline double pmax(double a, double b) { return a > b ? a : b; }
inline double pabs(double a) { return std::fabs(a); }
inline double pneg(double a) { return -a; }