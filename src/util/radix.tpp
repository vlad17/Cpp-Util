/*
  Vladimir Feinberg
  util/radix.tpp
  2015-04-07

  Implements radix sort.

  Resources used:
  http://algs4.cs.princeton.edu/51radix/MSD.java.html
  http://en.wikipedia.org/wiki/American_flag_sort
*/

#include "util/radix.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include "util/uassert.hpp"

namespace util {

namespace internal {

// Recursive MSD implementation that relies on the external radix arrays.
// See documentation of msd_in_place_radix() for templates description.
// Recall 'digit_of' returns '-1' as an end-of-key indicator.
//
// We are able to reuse the 'counts' and 'num_remaining' arrays between stack
// frames because this method returns the index it left off at (which it
// can compute by looking backwards in the array).
template<std::size_t Radix, typename RandomIt, typename DigitAt>
RandomIt msd_in_place_recursive(RandomIt first, RandomIt end, DigitAt digit_of,
                                int index,
                                std::array<int, (Radix + 2)>& counts,
                                std::array<int, (Radix + 1)>& num_remaining) {
  // TODO: potential optimization: cutoff to insertion sort (may
  // require a comparator for ints...)

  // TODO: merge this loop with below?
  if (first == end) return end;
  auto last = first;
  if (index > 0) {
    auto prev = index - 1;
    auto digit = digit_of(prev, *last++);
    UASSERT(-1 <= digit && digit < static_cast<int>(Radix))
      << "digit index " << digit << " not in range [-1, " << Radix << ")";
    while (last != end && digit_of(prev, *last) == digit) ++last;
  } else last = end;

  // Clear arrays
  std::fill(counts.begin(), counts.end(), 0);
  std::fill(num_remaining.begin(), num_remaining.end(), 0);

  // Find the counts of each digit. Note we offset by one here so we
  // may compute the index start for the *next-next* bucket.
  // The double shift allows for the -1 indicator value.
  for (auto i = first; i != last; ++i) {
    auto digit = digit_of(index, *i) + 1;
    UASSERT(0 <= digit && digit <= static_cast<int>(Radix))
      << "1 + digit index " << digit << " not in range [0, " << Radix << "]";
    ++counts[digit + 1];
    ++num_remaining[digit];
  }

  // Compute each bucket's offset.
  std::partial_sum(counts.begin(), counts.end(), counts.begin());

  for (std::size_t bucket = 0; bucket < Radix; ++bucket)
    while (num_remaining[bucket] > 0) {
      auto i = counts[bucket];
      auto target = digit_of(index, first[i]) + 1;
      UASSERT(0 <= target && target <= static_cast<int>(Radix))
        << "1 + digit index " << target << " not in range [0, " << Radix << "]";
      std::swap(first[i], first[counts[target]]);
      ++counts[target];
      --num_remaining[target];
    }

  auto next = first + counts[0]; // skip ended keys
  do next = msd_in_place_recursive<Radix>(next, last, digit_of, index + 1,
                                          counts, num_remaining);
  while (next != last);
  return next;
}

} // namespace internal

template<std::size_t Radix, typename RandomIt, typename DigitAt>
void msd_in_place_radix(RandomIt first, RandomIt last, DigitAt digit_of) {
  std::array<int, (Radix + 2)> counts;
  // TODO: potential optimization would be to remove the 'num_remaining'
  // in favor of keeping a single variable 'i' that tracks progress of the
  // current sweep. See wikipedia implementation for details.
  std::array<int, (Radix + 1)> num_remaining;
  internal::msd_in_place_recursive<Radix>(
      first, last, digit_of, 0, counts, num_remaining);
}

  // TODO: non-recursive based msd in place radix to beat built-in string sort?
  // TODO: how does it compare to three-way radix?

} // namespace util
