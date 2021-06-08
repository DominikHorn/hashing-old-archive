#pragma once

#include <string>

#include <convenience.hpp>

#include "pgm/include/pgm/pgm_index.hpp"

template<typename T,
         size_t Epsilon,
         size_t EpsilonRecursive,
         const size_t MaxModels = std::numeric_limits<size_t>::max(),
         typename Floating = float>
struct PGMHash : public pgm::PGMIndex<T, Epsilon, EpsilonRecursive, Floating> {
  private:
   using Parent = pgm::PGMIndex<T, Epsilon, EpsilonRecursive, Floating>;

   const T first_key;
   const size_t sample_size;
   const size_t N;

  public:
   /**
    * Constructs based on the sorted keys in the range [first, last). Note that
    * contrary to PGMIndex, a sample of the keys suffices.
    *
    * @param sample_begin, sample_end the range containing the sorted (!) keys to be indexed
    * @param full_size the output range of the hash function [0, full_size)
    */
   template<typename RandomIt>
   PGMHash(const RandomIt& sample_begin, const RandomIt& sample_end, const size_t full_size)
      : Parent(sample_begin, sample_end), first_key(*sample_begin),
        sample_size(std::distance(sample_begin, sample_end)), N(full_size) {
      if (this->segments.size() > MaxModels) {
         throw std::runtime_error("PGM " + name() + " had more models than allowed: " +
                                  std::to_string(this->segments.size()) + " > " + std::to_string(MaxModels));
      }
   }

   size_t model_count() {
      return this->segments.size();
   }

   /**
    * Human readable name useful, e.g., to log measured results
    * @return
    */
   static std::string name() {
      return "pgm_hash_eps" + std::to_string(Epsilon) + "_epsrec" + std::to_string(EpsilonRecursive);
   }

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
   forceinline Result operator()(const T& key) const {
      // Otherwise pgm will EXC_BAD_ACCESS
      if (unlikely(key == std::numeric_limits<HASH_64>::max())) {
         return N;
      }

      auto k = std::max(first_key, key);
      auto it = segment_for_key(k);

      // compute estimated pos (contrary to standard PGM, don't just throw slope precision away)
      const auto first_key_in_segment = it->key;
      auto segment_pos = static_cast<Precision>(it->slope * (k - first_key_in_segment)) + it->intercept;
      Precision relative_pos = segment_pos / static_cast<double>(sample_size);
      auto global_pos = static_cast<Result>(static_cast<Precision>(N) * relative_pos);

      // TODO: standard pgm algorithm limits clamped segment pos to at max intercept of next
      //    slope segment. Maybe we should do something similar?
      //         auto pos = std::min<size_t>((*it)(k), std::next(it)->intercept);

      return global_pos;
   }

  private:
    forceinline auto segment_for_key(const T &key) const {
        if constexpr (EpsilonRecursive == 0) {
            auto it = std::upper_bound(this->segments.begin(), this->segments.begin() + this->segments_count(), key);
            return it == this->segments.begin() ? it : std::prev(it);
        }

        auto it = this->segments.begin() + *(this->levels_offsets.end() - 2);
        for (auto l = int(this->height()) - 2; l >= 0; --l) {
            auto level_begin = this->segments.begin() + this->levels_offsets[l];
            auto pos = std::min<size_t>((*it)(key), std::next(it)->intercept);
            auto lo = level_begin + PGM_SUB_EPS(pos, EpsilonRecursive + 1);

            static constexpr size_t linear_search_threshold = 8 * 64 / sizeof(this->Segment);
            if constexpr (EpsilonRecursive <= linear_search_threshold) {
                for (; std::next(lo)->key <= key; ++lo)
                    continue;
                it = lo;
            } else {
                auto level_size = this->levels_offsets[l + 1] - this->levels_offsets[l] - 1;
                auto hi = level_begin + PGM_ADD_EPS(pos, EpsilonRecursive, level_size);
                it = std::upper_bound(lo, hi, key);
                it = it == level_begin ? it : std::prev(it);
            }
        }
        return it;
    }
};
