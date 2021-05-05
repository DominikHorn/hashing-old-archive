#pragma once

#include <convenience.hpp>

namespace Hashtable {
   template<class Key,
            class Payload,
            size_t BucketSize,
            class HashFn,
            class ReductionFn,
            class ProbingFn,
            Key Sentinel = std::numeric_limits<Key>::max()>
   struct Probing {
     private:
      const HashFn hashfn;
      const ReductionFn reductionfn;
      const ProbingFn probingfn;

     public:
      explicit Probing(const size_t& capacity)
         : hashfn(HashFn()), reductionfn(ReductionFn(num_buckets(capacity))), probingfn(ProbingFn()),
           slots(num_buckets(capacity)) {
         // Start with a well defined clean slate
         clear();
      };

      Probing(Probing&&) = default;

      /**
       * Inserts a key, value/payload pair into the hashtable
       *
       * Note: Will throw a runtime error iff the probing function produces a
       * cycle and all buckets along that cycle are full.
       *
       * @param key
       * @param payload
       * @return whether or not the key, payload pair was inserted. Insertion will fail
       *    iff the same key already exists or if key == Sentinel value
       */
      bool insert(const Key& key, const Payload payload) {
         if (unlikely(key == Sentinel)) {
            assert(false); // TODO: this must never happen in practice
            return false;
         }

         // Using template functor should successfully inline actual hash computation
         const auto orig_slot_index = reductionfn(hashfn(key));
         auto slot_index = orig_slot_index;
         size_t probing_step = 0;

         for (;;) {
            auto& slot = slots[slot_index];
            for (size_t i = 0; i < BucketSize; i++) {
               if (slot.keys[i] == Sentinel) {
                  slot.keys[i] = key;
                  slot.payloads[i] = payload;
                  return true;
               } else if (slot.keys[i] == key) {
                  // key already exists
                  return false;
               }
            }

            // Slot is full, choose a new slot index based on probing function
            slot_index = probingfn(orig_slot_index, ++probing_step, this->directory_size());
            if (unlikely(slot_index == orig_slot_index))
               throw std::runtime_error("Building " + this->name() +
                                        " failed: detected cycle during probing, all buckets along the way are full");
         }
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
         const auto orig_slot_index = reductionfn(hashfn(key));
         auto slot_index = orig_slot_index;
         size_t probing_step = 0;

         for (;;) {
            auto& slot = slots[slot_index];
            for (size_t i = 0; i < BucketSize; i++) {
               if (slot.keys[i] == key)
                  return std::make_optional(slot.payloads[i]);

               if (slot.keys[i] == Sentinel)
                  return std::nullopt;
            }

            // Slot is full, choose a new slot index based on probing function
            slot_index = probingfn(orig_slot_index, ++probing_step, this->directory_size());
            if (unlikely(slot_index == orig_slot_index))
               return std::nullopt;
         }
      }

      static constexpr forceinline size_t bucket_byte_size() {
         return sizeof(Bucket);
      }

      static forceinline std::string name() {
         return ProbingFn::name() + "_probing";
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

      /**
       * Clears all keys from the hashtable. Note that payloads are technically
       * still in memory (i.e., might leak if sensitive).
       */
      void clear() {
         for (auto& slot : slots) {
            slot.keys.fill(Sentinel);
         }
      }

      ~Probing() {
         clear();
      }

     protected:
      struct Bucket {
         std::array<Key, BucketSize> keys __attribute((aligned(sizeof(Key) * 8))) = {};
         std::array<Payload, BucketSize> payloads = {};
      } packed;

      std::vector<Bucket> slots;

      static constexpr forceinline size_t num_buckets(const size_t& capacity) {
         return capacity;
      }

      forceinline size_t directory_size() const {
         return slots.size();
      }
   };
} // namespace Hashtable