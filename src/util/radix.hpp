/*
  Vladimir Feinberg
  util/radix.hpp
  2015-04-07

  Describes the interface for an in-place radix sort, the American flag sort.
*/

#ifndef UTIL_RADIX_HPP_
#define UTIL_RADIX_HPP_

namespace util {

// Radix should be strictly greater than 0. DigitAt should be a type of a
// functor which should have a method that accepts a reference to an element
// pointed to by the iterator and returns an integer in the range [-1, Radix).
//
// Note that this allows for variable length keys - the DigitAt function should
// just return '-1' for an index past the end of the key, and lower values
// for other digits.
//
// The signature of the digit function should be compatible with the following:
//
//   int DigitAt::operator()(int, const T&)
//
// where T = decltype(*first), the first argument is the index (in the case
// of msd - most significant digit - this is the index reading left-to-right).
// The integer passed to the above operator will never be negative.
// 'digit_of' should be pure from the sort's perspective.
template<size_t Radix, typename RandomIt, typename DigitAt>
void msd_in_place_radix(RandomIt first, RandomIt last, DigitAt digit_of);

} // namespace util

#include "util/radix.tpp"

#endif /* UTIL_RADIX_HPP_ */
