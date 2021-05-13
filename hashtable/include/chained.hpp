#pragma once

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <vector>

#include <convenience.hpp>

namespace Hashtable {
   template<class Key, class Payload, size_t BucketSize, class HashFn, class ReductionFn,
            Key Sentinel = std::numeric_limits<Key>::max()>
   struct Chained {
     private:
      const HashFn hashfn;
      const ReductionFn reductionfn;
      const size_t capacity;
      bool allocated = false;

     public:
      explicit Chained(const size_t& capacity, const HashFn hashfn = HashFn())
         : hashfn(hashfn), reductionfn(ReductionFn(directory_address_count(capacity))), capacity(capacity){};

      Chained(Chained&&) = default;

      /**
       * Allocates hashtable memory. For production use, we would presumably call this in the constructor,
       * however this way we can save some time in case a measurement for this hashtable already exists.
       */
      void allocate() {
         if (allocated)
            return;
         allocated = true;

         slots.resize(directory_address_count(capacity));
         clear();
      }

      /**
       * Inserts a key, value/payload pair into the hashtable
       *
       * @param key
       * @param payload
       * @return whether or not the key, payload pair was inserted. Insertion will fail
       *    iff the same key already exists or if key == Sentinel value
       */
      bool insert(const Key& key, const Payload& payload) {
         if (unlikely(key == Sentinel)) {
            assert(false); // TODO: this must never happen in practice
            return false;
         }

         // Using template functor should successfully inline actual hash computation
         Slot& slot = slots[reductionfn(hashfn(key))];

         // Store directly in slot if possible
         if (slot.key == Sentinel) {
            slot.key = key;
            slot.payload = payload;
            return true;
         }

         // Initialize bucket chain if empty
         Bucket* bucket = slot.buckets;
         if (bucket == nullptr) {
            auto b = new Bucket();
            b->keys[0] = key;
            b->payloads[0] = payload;
            slot.buckets = b;
            return true;
         }

         // Go through existing buckets and try to insert there if possible
         for (;;) {
            // Find suitable empty entry place. Note that deletions with holes will require
            // searching entire bucket to deal with duplicate keys!
            for (size_t i = 0; i < BucketSize; i++) {
               if (bucket->keys[i] == Sentinel) {
                  bucket->keys[i] = key;
                  bucket->payloads[i] = payload;
                  return true;
               } else if (bucket->keys[i] == key) {
                  // key already exists
                  return false;
               }
            }

            if (bucket->next == nullptr)
               break;
            bucket = bucket->next;
         }

         // Append a new bucket to the chain and add element there
         auto b = new Bucket();
         b->keys[0] = key;
         b->payloads[0] = payload;
         bucket->next = b;
         return true;
      }

      /**
       * Retrieves the associated payload/value for a given key.
       *
       * @param key
       * @return the payload or std::nullopt if key was not found in the Hashtable
       */
      std::optional<Payload> lookup(const Key& key) const {
         if (unlikely(key == Sentinel)) {
            assert(false); // TODO: this must never happen in practice
            return std::nullopt;
         }

         // Using template functor should successfully inline actual hash computation
         const Slot& slot = slots[reductionfn(hashfn(key))];

         if (slot.key == key) {
            return std::make_optional(slot.payload);
         }

         Bucket* bucket = slot.buckets;
         while (bucket != nullptr) {
            for (size_t i = 0; i < BucketSize; i++) {
               if (bucket->keys[i] == key)
                  return std::make_optional(bucket->payloads[i]);

               if (bucket->keys[i] == Sentinel)
                  return std::nullopt;
            }
            bucket = bucket->next;
         }

         return std::nullopt;
      }

      static constexpr forceinline size_t bucket_byte_size() {
         return sizeof(Bucket);
      }

      static constexpr forceinline size_t slot_byte_size() {
         return sizeof(Slot);
      }

      static forceinline std::string name() {
         return "chained";
      }

      static forceinline std::string hash_name() {
         return HashFn::name();
      }

      static forceinline std::string reducer_name() {
         return ReductionFn::name();
      }

      static constexpr forceinline size_t bucket_size() {
         return BucketSize;
      }

      static constexpr forceinline size_t directory_address_count(const size_t& capacity) {
         return capacity;
      }

      /**
       * Clears all keys from the hashtable. Note that payloads are technically
       * still in memory (i.e., might leak if sensitive).
       */
      void clear() {
         for (auto& slot : slots) {
            slot.key = Sentinel;

            auto bucket = slot.buckets;
            slot.buckets = nullptr;

            while (bucket != nullptr) {
               auto next = bucket->next;
               delete bucket;
               bucket = next;
            }
         }
      }

      ~Chained() {
         clear();
      }

     protected:
      struct Bucket {
         std::array<Key, BucketSize> keys /*__attribute((aligned(sizeof(Key) * 8)))*/;
         std::array<Payload, BucketSize> payloads;
         Bucket* next = nullptr;
      } packed;

      struct Slot {
         Key key;
         Payload payload;
         Bucket* buckets = nullptr;
      } packed;

      // First bucket is always inline in the slot
      std::vector<Slot> slots;
   };
} // namespace Hashtable
