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
#include <optional>
#include <random>
#include <vector>
#include <immintrin.h>

#include <convenience.hpp>

namespace Hashtable {
   template<typename Key, typename Payload, size_t BucketSize = 1, Key Sentinel = std::numeric_limits<Key>::max()>
   class Cuckoo {
     private:
      struct Bucket {
         Key keys[BucketSize] __attribute((aligned(32)));
         Payload values[BucketSize];
      } packed;

      Bucket* buckets_;
      size_t size_; // Number of entries filled
      size_t num_buckets_; // Total number of buckets
      std::mt19937 rand_; // RNG for moving items around

     public:
      Cuckoo(size_t capacity) : size_(0), num_buckets_((capacity + BucketSize - 1) / BucketSize) {
         int r = posix_memalign(reinterpret_cast<void**>(&buckets_), 32, num_buckets_ * sizeof(Bucket));
         if (r != 0)
            throw std::runtime_error("Could not memalign allocate for cuckoo hash map");

         for (size_t i = 0; i < num_buckets_; i++) {
            for (size_t j = 0; j < BucketSize; j++) {
               buckets_[i].keys[j] = Sentinel;
            }
         }
      }

      ~Cuckoo() {
         free(buckets_);
      }

      template<typename Hash1, typename Hash2, typename Reducer>
      std::optional<Payload> lookup(const Key& key, const Hash1& hashfn1, const Hash2& hashfn2,
                                    const Reducer& reducer) const {
         const auto h1 = hashfn1(key);
         const auto i1 = reducer(h1, num_buckets_);

         Bucket* b1 = &buckets_[i1];
         for (size_t i = 0; i < BucketSize; i++) {
            if (b1->keys[i] == key) {
               return {b1->values[i]};
            }
         }

         auto i2 = reducer(hashfn2(key, h1), num_buckets_);
         if (i2 == i1) {
            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
         }

         Bucket* b2 = &buckets_[i2];
         for (size_t i = 0; i < BucketSize; i++) {
            if (b2->keys[i] == key) {
               return {b2->values[i]};
            }
         }

         return std::nullopt;
      }

      template<typename Hash1, typename Hash2, typename Reducer>
      void insert(const Key& key, const Payload& value, const Hash1& hashfn1, const Hash2& hashfn2,
                  const Reducer& reducer) {
         insert(key, value, hashfn1, hashfn2, reducer, false);
      }

      forceinline size_t size() {
         return size_;
      }

      static constexpr forceinline size_t bucket_size() {
         return sizeof(Bucket);
      }

      void clear() {
         for (auto& bucket : buckets_) {
            for (size_t i = 0; i < BucketSize; i++) {
               bucket.key[i] = Sentinel;
            }
         }
      }

     private:
      template<typename Hash1, typename Hash2, typename Reducer>
      void insert(const Key& key, const Payload& value, const Hash1& hashfn1, const Hash2& hashfn2,
                  const Reducer& reducer, bool is_reinsert) {
         const auto h1 = hashfn1(key);
         const auto i1 = reducer(h1, num_buckets_);
         auto i2 = reducer(hashfn2(key, h1), num_buckets_);

         if (unlikely(i2 == i1)) {
            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
         }

         Bucket* b1 = &buckets_[i1];
         Bucket* b2 = &buckets_[i2];

         // Update old value if the key is already in the table
         for (size_t i = 0; i < BucketSize; i++) {
            if (key == b1->keys[i]) {
               b1->values[i] = value;
               return;
            }
            if (key == b2->keys[i]) {
               b2->values[i] = value;
               return;
            }
         }

         if (!is_reinsert) {
            size_++;
         }

         size_t count1 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            count1 += (b1->keys[i] != Sentinel ? 1 : 0);
         }
         size_t count2 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            count2 += (b2->keys[i] != Sentinel ? 1 : 0);
         }

         if (count1 <= count2 && count1 < BucketSize) {
            // Add it into bucket 1
            b1->keys[count1] = key;
            b1->values[count1] = value;
         } else if (count2 < BucketSize) {
            // Add it into bucket 2
            b2->keys[count2] = key;
            b2->values[count2] = value;
         } else {
            // Both buckets are full; evict a random item from one of them
            assert(count1 == BucketSize);
            assert(count2 == BucketSize);

            Bucket* victim_bucket = b1;
            if ((rand_() & 0x01) == 0) {
               victim_bucket = b2;
            }
            uint32_t victim_index = rand_() % BucketSize;
            uint32_t old_key = victim_bucket->keys[victim_index];
            Payload old_value = victim_bucket->values[victim_index];
            victim_bucket->keys[victim_index] = key;
            victim_bucket->values[victim_index] = value;
            insert(old_key, old_value, hashfn1, hashfn2, reducer, true);
         }
      }
   };

   template<typename Payload, uint32_t Sentinel>
   class Cuckoo<uint32_t, Payload, 8, Sentinel> {
     private:
      static constexpr uint32_t BucketSize = 8;

      struct Bucket {
         uint32_t keys[BucketSize] __attribute((aligned(32)));
         Payload values[BucketSize];
      } packed;

      Bucket* buckets_;
      size_t size_; // Number of entries filled
      size_t num_buckets_; // Total number of buckets
      std::mt19937 rand_; // RNG for moving items around

     public:
      Cuckoo(size_t capacity) : size_(0), num_buckets_((capacity + 8 - 1) / 8) {
         int r = posix_memalign(reinterpret_cast<void**>(&buckets_), 32, num_buckets_ * sizeof(Bucket));
         if (r != 0)
            throw std::runtime_error("Could not memalign allocate for cuckoo hash map");

         for (size_t i = 0; i < num_buckets_; i++) {
            for (size_t j = 0; j < 8; j++) {
               buckets_[i].keys[j] = Sentinel;
            }
         }
      }

      ~Cuckoo() {
         free(buckets_);
      }

      template<typename Hash1, typename Hash2, typename Reducer>
      std::optional<Payload> lookup(const uint32_t& key, const Hash1& hashfn1, const Hash2& hashfn2,
                                    const Reducer& reducer) const {
         const auto h1 = hashfn1(key);
         const auto i1 = reducer(h1, num_buckets_);

         Bucket* b1 = &buckets_[i1];
         __m256i vkey = _mm256_set1_epi32(key);
         __m256i vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b1->keys));
         __m256i cmp = _mm256_cmpeq_epi32(vkey, vbucket);
         int mask = _mm256_movemask_epi8(cmp);
         if (mask != 0) {
            int index = __builtin_ctz(mask) / 4;
            return {b1->values[index]};
         }

         auto i2 = reducer(hashfn2(key, h1), num_buckets_);
         if (i2 == i1) {
            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
         }
         Bucket* b2 = &buckets_[i2];
         vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b2->keys));
         cmp = _mm256_cmpeq_epi32(vkey, vbucket);
         mask = _mm256_movemask_epi8(cmp);
         if (mask != 0) {
            int index = __builtin_ctz(mask) / 4;
            return {b2->values[index]};
         }

         return std::nullopt;
      }

      template<typename Hash1, typename Hash2, typename Reducer>
      void insert(const uint32_t& key, const Payload& value, const Hash1& hashfn1, const Hash2& hashfn2,
                  const Reducer& reducer) {
         insert(key, value, hashfn1, hashfn2, reducer, false);
      }

      forceinline size_t size() {
         return size_;
      }

      static constexpr forceinline size_t bucket_size() {
         return sizeof(Bucket);
      }

      void clear() {
         for (auto& bucket : buckets_) {
            for (size_t i = 0; i < BucketSize; i++) {
               bucket.key[i] = Sentinel;
            }
         }
      }

     private:
      template<typename Hash1, typename Hash2, typename Reducer>
      void insert(const uint32_t& key, const Payload& value, const Hash1& hashfn1, const Hash2& hashfn2,
                  const Reducer& reducer, bool is_reinsert) {
         const auto h1 = hashfn1(key);
         const auto i1 = reducer(h1, num_buckets_);
         auto i2 = reducer(hashfn2(key, h1), num_buckets_);

         if (unlikely(i2 == i1)) {
            i2 = (i1 == num_buckets_ - 1) ? 0 : i1 + 1;
         }

         Bucket* b1 = &buckets_[i1];
         Bucket* b2 = &buckets_[i2];

         // Update old value if the key is already in the table
         __m256i vkey = _mm256_set1_epi32(key);
         __m256i vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b1->keys));
         __m256i cmp = _mm256_cmpeq_epi32(vkey, vbucket);
         int mask = _mm256_movemask_epi8(cmp);
         if (mask != 0) {
            int index = __builtin_ctz(mask) / 4;
            b1->values[index] = value;
            return;
         }

         vbucket = _mm256_load_si256(reinterpret_cast<const __m256i*>(&b2->keys));
         cmp = _mm256_cmpeq_epi32(vkey, vbucket);
         mask = _mm256_movemask_epi8(cmp);
         if (mask != 0) {
            int index = __builtin_ctz(mask) / 4;
            b2->values[index] = value;
            return;
         }

         if (!is_reinsert) {
            size_++;
         }

         size_t count1 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            count1 += (b1->keys[i] != Sentinel ? 1 : 0);
         }
         size_t count2 = 0;
         for (size_t i = 0; i < BucketSize; i++) {
            count2 += (b2->keys[i] != Sentinel ? 1 : 0);
         }

         if (count1 <= count2 && count1 < BucketSize) {
            // Add it into bucket 1
            b1->keys[count1] = key;
            b1->values[count1] = value;
         } else if (count2 < BucketSize) {
            // Add it into bucket 2
            b2->keys[count2] = key;
            b2->values[count2] = value;
         } else {
            // Both buckets are full; evict a random item from one of them
            assert(count1 == BucketSize);
            assert(count2 == BucketSize);

            Bucket* victim_bucket = b1;
            if ((rand_() & 0x01) == 0) {
               victim_bucket = b2;
            }
            uint32_t victim_index = rand_() % BucketSize;
            uint32_t old_key = victim_bucket->keys[victim_index];
            Payload old_value = victim_bucket->values[victim_index];
            victim_bucket->keys[victim_index] = key;
            victim_bucket->values[victim_index] = value;
            insert(old_key, old_value, hashfn1, hashfn2, reducer, true);
         }
      }
   };
} // namespace Hashtable