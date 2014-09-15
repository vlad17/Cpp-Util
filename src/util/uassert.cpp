/*
  Vladimir Feinberg
  util/uassert.cpp
  2014-09-14

  Implementation of internal class for assert macro.
*/

#include "util/uassert.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;

#ifndef USE_UASSERT
#define USE_UASSERT !defined(NDEBUG)
#endif /* USE_UASSERT */

#if USE_UASSERT
using namespace util;
using namespace _util_uassert_internal;

namespace {
// Strips directory names bc they can be long. If separator is not '/'
// then they're not stripped.
const char* sansdirectory(const char* file) {
  const char* ret = strrchr(file, '/');
  return ret ? ret + 1 : file;
}
} // anonymous namespace

streamer::streamer(bool print, const char* file, long line, const char* expr) :
    print_(print),
    file_(sansdirectory(file)),
    line_(line),
    expr_(expr) {}

streamer::~streamer() {
  if (print_) {
    stringstream sstr;
    sstr << file_ << ":" << line_ << " Expr \'" << expr_ << "\' failed";
    string str = sstr_.str();
    if (!str.empty()) sstr << ": " << str;
    else sstr << ".";
    sstr << "\n";
    cerr << sstr.str();
    exit(EXIT_FAILURE);
  }
}

void streamer::unexpected() {
  cerr << "WARNING, an unexpected exception occured when "
      "attempting to stringify the error message for a failed "
      "assertion. Will attempt to skip and continue.\n";
}

#endif /* USE_UASSERT */
