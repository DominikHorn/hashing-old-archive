#pragma once

#include <convenience.hpp>

#include "pgm/include/pgm/pgm_index.hpp"

namespace pgm {
   template<typename K, size_t Epsilon = 64, typename Floating = float, size_t EpsilonRecursive = 4>
   class PGMHash : public PGMIndex<K, Epsilon, EpsilonRecursive, Floating> {
     protected:
      const K first_key;
      const size_t sample_size;

     public:
      /**
       * Constructs based on the sorted keys in the range [first, last). Note that
       * contrary to PGMIndex, a sample of the keys suffices.
       *
       * @param first, last the range containing the sorted (!) keys to be indexed
       */
      template<typename RandomIt>
      PGMHash(const RandomIt& first, const RandomIt& last)
         : PGMIndex<K, Epsilon, EpsilonRecursive, Floating>(first, last), first_key(*first),
           sample_size(std::distance(first, last)) {}

      /**
       * Computes a hash value aiming to be within [0, N] based
       * on the PGMIndex::search algorithm. Note that additional
       * reduction might be necessary to guarantee [0, N] bounds.
       *
       * Contrary to PGMIndex::search, the precision available through
       * segment->slope is not immediately thrown away and instead carried
       * into the scaling (from input keyset size to [0, N])
       * computation, which should result in significantly more unique
       * hash/pos values compared to standard PGMIndex::search() when
       * the index is only trained on a sample.
       *
       * @tparam Result
       * @tparam Precision
       * @param key
       * @param N
       * @return
       */
      template<typename Result = size_t, typename Precision = double>
      forceinline Result hash(const K& key, const Result& N) const {
         auto k = std::max(first_key, key);
         auto it = this->segment_for_key(k);

         // compute estimated pos (contrary to standard PGM, don't just throw slope precision away)
         auto segment_pos = static_cast<Precision>(it->slope * (k - key)) + it->intercept;
         Precision relative_pos = segment_pos / static_cast<double>(sample_size);
         auto global_pos = static_cast<Result>(static_cast<Precision>(N) * relative_pos);

         // TODO: standard pgm algorithm limits returned segment pos to at max intercept of next
         //    slope segment. Maybe we should do something similar?
         //         auto pos = std::min<size_t>((*it)(k), std::next(it)->intercept);

         return global_pos;
      }
   };
} // namespace pgm