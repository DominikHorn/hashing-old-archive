#pragma once

// This is a modified version of hashing.cc from SOSD repo (https://github.com/learnedsystems/SOSD),
// originally taken from the Stanford FutureData index baselines repo. Original copyright:
// Copyright (c) 2017-present Peter Bailis, Kai Sheng Tai, Pratiksha Thaker, Matei Zaharia
// MIT License

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <optional>
#include <random>
#include <vector>
#include <immintrin.h>

#include <convenience.hpp>

namespace Hashtable {
   /**
    * Place entry in bucket with more available space.
    * If both are full, kick from either bucket with 50% chance
    */
   struct BalancedKicking {
     private:
      std::mt19937 rand_;

     public:
      static std::string name() {
         return "balanced_kicking";
      }

      template<class Bucket, class Key, class Payload, size_t BucketSize, Key Sentinel>
      forceinline std::optional<std::pair<Key, Payload>> operator()(Bucket* b1, Bucket* b2, const Key& key,
                                                                    const Payload& payload) {
         size_t c1 = 0, c2 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            c1 += (b1->slots[i].key == Sentinel ? 0 : 1);
            c2 += (b2->slots[i].key == Sentinel ? 0 : 1);
         }

         if (c1 <= c2 && c1 < BucketSize) {
            b1->slots[c1] = {.key = key, .payload = payload};
            return std::nullopt;
         }

         if (c2 < BucketSize) {
            b2->slots[c2] = {.key = key, .payload = payload};
            return std::nullopt;
         }

         const auto rng = rand_();
         const auto victim_bucket = rng & 0x1 ? b1 : b2;
         const size_t victim_index = rng % BucketSize;
         Key victim_key = victim_bucket->slots[victim_index].key;
         Payload victim_payload = victim_bucket->slots[victim_index].payload;
         victim_bucket->slots[victim_index] = {.key = key, .payload = payload};
         return std::make_optional(std::make_pair(victim_key, victim_payload));
      };
   };

   /**
    * if primary bucket has space, place entry in there
    * else if secondary bucket has space, place entry in there
    * else kick a random entry from the primary bucket with chance
    *
    * @tparam Bias chance that element is kicked from second bucket in percent (i.e., value of 10 -> 10%)
    */
   template<uint8_t Bias>
   struct BiasedKicking {
     private:
      std::mt19937 rand_;
      double chance = static_cast<double>(Bias) / 100.0;
      uint_fast32_t threshold_ = std::numeric_limits<uint_fast32_t>::max() * chance;

     public:
      static std::string name() {
         return "biased_kicking_" + std::to_string(Bias);
      }

      template<class Bucket, class Key, class Payload, size_t BucketSize, Key Sentinel>
      forceinline std::optional<std::pair<Key, Payload>> operator()(Bucket* b1, Bucket* b2, const Key& key,
                                                                    const Payload& payload) {
         size_t c1 = 0, c2 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            c1 += (b1->slots[i].key == Sentinel ? 0 : 1);
            c2 += (b2->slots[i].key == Sentinel ? 0 : 1);
         }

         if (c1 < BucketSize) {
            b1->slots[c1] = {.key = key, .payload = payload};
            return std::nullopt;
         }

         if (c2 < BucketSize) {
            b2->slots[c2] = {.key = key, .payload = payload};
            return std::nullopt;
         }

         const auto rng = rand_();
         const auto victim_bucket = rng > threshold_ ? b1 : b2;
         const size_t victim_index = rng % BucketSize;
         Key victim_key = victim_bucket->slots[victim_index].key;
         Payload victim_payload = victim_bucket->slots[victim_index].payload;
         victim_bucket->slots[victim_index] = {.key = key, .payload = payload};
         return std::make_optional(std::make_pair(victim_key, victim_payload));
      };
   };

   /**
    * if primary bucket has space, place entry in there
    * else if secondary bucket has space, place entry in there
    * else kick a random entry from the primary bucket and place entry in primary bucket
    */
   using UnbiasedKicking = BiasedKicking<0>;

   template<class Key, class Payload, size_t BucketSize, class HashFn1, class HashFn2, class ReductionFn1,
            class ReductionFn2, class KickingFn, Key Sentinel = std::numeric_limits<Key>::max()>
   class Cuckoo {
     public:
      using KeyType = Key;
      using PayloadType = Payload;

     private:
      const size_t MaxKickCycleLength;
      const HashFn1 hashfn1;
      const HashFn2 hashfn2;
      const ReductionFn1 reductionfn1;
      const ReductionFn2 reductionfn2;
      KickingFn kickingfn;

      struct Bucket {
         struct Slot {
            Key key = Sentinel;
            Payload payload;
         } packed;

         std::array<Slot, BucketSize> slots;
      } packed;

      std::vector<Bucket> buckets;

      std::mt19937 rand_; // RNG for moving items around

     public:
      Cuckoo(const size_t& capacity, const HashFn1 hashfn1 = HashFn1(), const HashFn2 hashfn2 = HashFn2())
         : MaxKickCycleLength(capacity / 2), hashfn1(hashfn1), hashfn2(hashfn2),
           reductionfn1(ReductionFn1(directory_address_count(capacity))),
           reductionfn2(ReductionFn2(directory_address_count(capacity))), kickingfn(KickingFn()),
           buckets(directory_address_count(capacity)) {}

      std::optional<Payload> lookup(const Key& key) const {
         const auto h1 = hashfn1(key);
         const auto i1 = reductionfn1(h1);

         const Bucket* b1 = &buckets[i1];
         for (size_t i = 0; i < BucketSize; i++) {
            if (b1->slots[i].key == key) {
               Payload payload = b1->slots[i].payload;
               return std::make_optional(payload);
            }
         }

         auto i2 = reductionfn2(hashfn2(key, h1));
         if (i2 == i1) {
            i2 = (i1 == buckets.size() - 1) ? 0 : i1 + 1;
         }

         const Bucket* b2 = &buckets[i2];
         for (size_t i = 0; i < BucketSize; i++) {
            if (b2->slots[i].key == key) {
               Payload payload = b2->slots[i].payload;
               return std::make_optional(payload);
            }
         }

         return std::nullopt;
      }

      std::map<std::string, std::string> lookup_statistics(const std::vector<Key>& dataset) const {
         size_t primary_key_cnt = 0;

         for (const auto& key : dataset) {
            const auto h1 = hashfn1(key);
            const auto i1 = reductionfn1(h1);

            const Bucket* b1 = &buckets[i1];
            for (size_t i = 0; i < BucketSize; i++)
               if (b1->slots[i].key == key)
                  primary_key_cnt++;
         }

         return {
            {"primary_key_ratio",
             std::to_string(static_cast<long double>(primary_key_cnt) / static_cast<long double>(dataset.size()))},
         };
      }

      void insert(const Key& key, const Payload& value) {
         insert(key, value, 0);
      }

      static constexpr forceinline size_t bucket_byte_size() {
         return sizeof(Bucket);
      }

      static forceinline std::string name() {
         return "cuckoo_" + std::to_string(BucketSize) + "_" + KickingFn::name();
      }

      static forceinline std::string hash_name() {
         return HashFn1::name() + "-" + HashFn2::name();
      }

      static forceinline std::string reducer_name() {
         return ReductionFn1::name() + "-" + ReductionFn2::name();
      }

      static constexpr forceinline size_t bucket_size() {
         return BucketSize;
      }

      static constexpr forceinline size_t directory_address_count(const size_t& capacity) {
         return (capacity + BucketSize - 1) / BucketSize;
      }

      void clear() {
         for (auto& bucket : buckets)
            for (auto& slot : bucket.slots)
               slot.key = Sentinel;
      }

     private:
      void insert(Key key, Payload payload, size_t kick_count) {
      start:
         // TODO: track max kick_count for result graphs
         if (kick_count > MaxKickCycleLength) {
            throw std::runtime_error("maximum kick cycle length (" + std::to_string(MaxKickCycleLength) + ") reached");
         }

         const auto h1 = hashfn1(key);
         const auto i1 = reductionfn1(h1);
         auto i2 = reductionfn2(hashfn2(key, h1));

         if (unlikely(i2 == i1)) {
            i2 = (i1 == buckets.size() - 1) ? 0 : i1 + 1;
         }

         Bucket* b1 = &buckets[i1];
         Bucket* b2 = &buckets[i2];

         // Update old value if the key is already in the table
         for (size_t i = 0; i < BucketSize; i++) {
            if (b1->slots[i].key == key) {
               b1->slots[i].payload = payload;
               return;
            }
            if (b2->slots[i].key == key) {
               b2->slots[i].payload = payload;
               return;
            }
         }

         // Way to go Mr. Stroustrup
         if (const auto kicked =
                kickingfn.template operator()<Bucket, Key, Payload, BucketSize, Sentinel>(b1, b2, key, payload)) {
            key = kicked.value().first;
            payload = kicked.value().second;
            kick_count++;
            goto start;
         }
      }
   };

#warning "Stanford uint32_t vectorized cuckoo is not enabled"
   //   template<class Payload, class HashFn1, class HashFn2, class ReductionFn1, class ReductionFn2, class KickingFn,
   //            uint32_t Sentinel>
   //   class Cuckoo<uint32_t, Payload, 8, HashFn1, HashFn2, ReductionFn1, ReductionFn2, KickingFn, Sentinel> {
   //     public:
   //      typedef uint32_t KeyType;
   //      typedef Payload PayloadType;
   //
   //     private:
   //      static constexpr uint32_t BucketSize = 8;
   //      const size_t MaxKickCycleLength;
   //
   //      const HashFn1 hashfn1;
   //      const HashFn2 hashfn2;
   //      const ReductionFn1 reductionfn1;
   //      const ReductionFn2 reductionfn2;
   //      KickingFn kickingfn;
   //
   //      struct Bucket {
   //         uint32_t keys[BucketSize] __attribute((aligned(32)));
   //         Payload values[BucketSize];
   //      } packed;
   //
   //      Bucket* buckets_;
   //      size_t num_buckets_; // Total number of buckets
   //
   //      std::mt19937 rand_; // RNG for moving items around
   //
   //     public:
   //   Cuckoo(const size_t& capacity)
   //      : MaxKickCycleLength(4096), hashfn1(HashFn1()), hashfn2(HashFn2()),
   //        reductionfn1(ReductionFn1(directory_address_count(capacity))),
   //        reductionfn2(ReductionFn2(directory_address_count(capacity))), kickingfn(KickingFn()),
   //        num_buckets_(directory_address_count(capacity)) {
   //      // Allocate memory
   //      int r = posix_memalign(reinterpret_cast<void**>(&buckets_), 32, num_buckets_ * sizeof(Bucket));
   //      if (r != 0)
   //         throw std::runtime_error("Could not memalign allocate for cuckoo hash map");
   //
   //      // Ensure all slots are in cleared state
   //      clear();
   //   }
   //
   //      ~Cuckoo() {
   //         free(buckets_);
   //      }
   //
   //      std::optional<Payload> lookup(const uint32_t& key) const {
   //         const auto h1 = hashfn1(key);
   //         const auto i1 = reductionfn1(h1);
   //
   //         Bucket* b1 = &buckets_[i1];
   //         __m256i vkey = _mm256_set1_epi32(key);
   //         __m256i vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b1->keys));
   //         __m256i cmp = _mm256_cmpeq_epi32(vkey, vbucket);
   //         int mask = _mm256_movemask_epi8(cmp);
   //         if (mask != 0) {
   //            int index = __builtin_ctz(mask) / 4;
   //            auto val = b1->values[index];
   //            return std::make_optional(val);
   //         }
   //
   //         auto i2 = reductionfn2(hashfn2(key, h1));
   //         if (i2 == i1) {
   //            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
   //         }
   //         Bucket* b2 = &buckets_[i2];
   //         vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b2->keys));
   //         cmp = _mm256_cmpeq_epi32(vkey, vbucket);
   //         mask = _mm256_movemask_epi8(cmp);
   //         if (mask != 0) {
   //            int index = __builtin_ctz(mask) / 4;
   //            auto val = b2->values[index];
   //            return std::make_optional(val);
   //         }
   //
   //         return std::nullopt;
   //      }
   //
   //      std::map<std::string, std::string> lookup_statistics(const std::vector<KeyType>& dataset) {
   //         size_t primary_key_cnt;
   //
   //         for (const auto& key : dataset) {
   //            const auto h1 = hashfn1(key);
   //            const auto i1 = reductionfn1(h1);
   //
   //            Bucket* b1 = &buckets_[i1];
   //            __m256i vkey = _mm256_set1_epi32(key);
   //            __m256i vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b1->keys));
   //            __m256i cmp = _mm256_cmpeq_epi32(vkey, vbucket);
   //            int mask = _mm256_movemask_epi8(cmp);
   //            if (mask != 0) {
   //               primary_key_cnt++;
   //            }
   //         }
   //
   //         return {
   //            {"primary_key_ratio",
   //             std::to_string(static_cast<long double>(primary_key_cnt) / static_cast<long double>(dataset.size()))},
   //         };
   //      }
   //
   //      void insert(const uint32_t& key, const Payload& value) {
   //         insert(key, value, 0);
   //      }
   //
   //      static constexpr forceinline size_t bucket_byte_size() {
   //         return sizeof(Bucket);
   //      }
   //
   //      static forceinline std::string name() {
   //         return "simd_cuckoo_" + KickingFn::name();
   //      }
   //
   //      static forceinline std::string hash_name() {
   //         return HashFn1::name() + "-" + HashFn2::name();
   //      }
   //
   //      static forceinline std::string reducer_name() {
   //         return ReductionFn1::name() + "-" + ReductionFn2::name();
   //      }
   //
   //      static constexpr forceinline size_t bucket_size() {
   //         return BucketSize;
   //      }
   //
   //      static constexpr forceinline size_t directory_address_count(const size_t& capacity) {
   //         return (capacity + BucketSize - 1) / BucketSize;
   //      }
   //
   //      void clear() {
   //         for (size_t i = 0; i < num_buckets_; i++) {
   //            for (size_t j = 0; j < BucketSize; j++) {
   //               buckets_[i].keys[j] = Sentinel;
   //            }
   //         }
   //      }
   //
   //     private:
   //      void insert(const uint32_t& key, const Payload& value, size_t kick_count) {
   //         // TODO: track max kick_count for result graphs
   //         if (kick_count > MaxKickCycleLength) {
   //            throw std::runtime_error("maximum kick cycle length (" + std::to_string(MaxKickCycleLength) + ") reached");
   //         }
   //
   //         const auto h1 = hashfn1(key);
   //         const auto i1 = reductionfn1(h1);
   //         auto i2 = reductionfn2(hashfn2(key, h1));
   //
   //         if (unlikely(i2 == i1)) {
   //            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
   //         }
   //
   //         Bucket* b1 = &buckets_[i1];
   //         Bucket* b2 = &buckets_[i2];
   //
   //         // Update old value if the key is already in the table
   //         __m256i vkey = _mm256_set1_epi32(key);
   //         __m256i vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b1->keys));
   //         __m256i cmp = _mm256_cmpeq_epi32(vkey, vbucket);
   //         int mask = _mm256_movemask_epi8(cmp);
   //         if (mask != 0) {
   //            int index = __builtin_ctz(mask) / 4;
   //            b1->values[index] = value;
   //            return;
   //         }
   //
   //         vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b2->keys));
   //         cmp = _mm256_cmpeq_epi32(vkey, vbucket);
   //         mask = _mm256_movemask_epi8(cmp);
   //         if (mask != 0) {
   //            int index = __builtin_ctz(mask) / 4;
   //            b2->values[index] = value;
   //            return;
   //         }
   //
   //         // Way to go Mr. Stroustrup
   //         if (const auto kicked =
   //                kickingfn.template operator()<Bucket, uint32_t, Payload, BucketSize, Sentinel>(b1, b2, key, value)) {
   //            insert(kicked.value().first, kicked.value().second, kick_count + 1);
   //         }
   //      }
   //   };
} // namespace Hashtable