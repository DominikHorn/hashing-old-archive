#pragma once

#include <map>
#include <vector>

#include <convenience.hpp>
#include <reduction.hpp>
#include <thirdparty/libdivide.h>

namespace Hashtable {
   struct LinearProbingFunc {
     private:
      const size_t directory_size;

     public:
      LinearProbingFunc(const size_t& directory_size) : directory_size(directory_size) {}

      static std::string name() {
         return "linear";
      }

      forceinline size_t operator()(const size_t& index, const size_t& probing_step) const {
         auto next = index + probing_step;
         // TODO: benchmark whether this really is the fastest implementation
         while (unlikely(next >= directory_size))
            next -= directory_size;
         return next;
      }
   };

   struct QuadraticProbingFunc {
     private:
      const size_t directory_size;
      const libdivide::divider<size_t> div;

     public:
      QuadraticProbingFunc(const size_t& directory_size)
         : directory_size(directory_size), div(Reduction::make_magic_divider(directory_size)) {}

      static std::string name() {
         return "quadratic";
      }

      forceinline size_t operator()(const size_t& index, const size_t& probing_step) const {
         return Reduction::magic_modulo(index + probing_step * probing_step, directory_size, div);
      }
   };

   template<class Key,
            class Payload,
            class HashFn,
            class ReductionFn,
            class ProbingFn,
            size_t BucketSize = 1,
            Key Sentinel = std::numeric_limits<Key>::max()>
   struct Probing {
     public:
      using KeyType = Key;
      using PayloadType = Payload;

     private:
      const HashFn hashfn;
      const ReductionFn reductionfn;
      const ProbingFn probingfn;
      const size_t capacity;
      bool allocated = false;

     public:
      explicit Probing(const size_t& capacity, const HashFn hashfn = HashFn())
         : hashfn(hashfn), reductionfn(ReductionFn(directory_address_count(capacity))),
           probingfn(ProbingFn(directory_address_count(capacity))), capacity(capacity){};

      Probing(Probing&&) = default;

      /**
       * Allocates hashtable memory. For production use, we would presumably call this in the constructor,
       * however this way we can save some time in case a measurement for this hashtable already exists.
       */
      void allocate() {
         if (allocated)
            return;
         allocated = true;

         // Reserve required memory
         slots.resize(directory_address_count(capacity));
         // Ensure all slots are in cleared state
         clear();
      }

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
            slot_index = probingfn(orig_slot_index, ++probing_step);
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
            slot_index = probingfn(orig_slot_index, ++probing_step);
            if (unlikely(slot_index == orig_slot_index))
               return std::nullopt;
         }
      }

      std::map<std::string, std::string> lookup_statistics(const std::vector<Key>& dataset) {
         size_t min_psl, max_psl, total_psl = 0;

         for (const auto& key : dataset) {
            // Using template functor should successfully inline actual hash computation
            const auto orig_slot_index = reductionfn(hashfn(key));
            auto slot_index = orig_slot_index;
            size_t probing_step = 0;

            for (;;) {
               auto& slot = slots[slot_index];
               for (size_t i = 0; i < BucketSize; i++) {
                  if (slot.keys[i] == key) {
                     min_psl = std::min(min_psl, probing_step);
                     max_psl = std::max(max_psl, probing_step);
                     total_psl += probing_step;
                     goto next;
                  }

                  if (slot.keys[i] == Sentinel)
                     goto next;
               }

               // Slot is full, choose a new slot index based on probing function
               slot_index = probingfn(orig_slot_index, ++probing_step);
               if (unlikely(slot_index == orig_slot_index))
                  goto next;
            }

         next:
            continue;
         }

         return {{"min_psl", std::to_string(min_psl)},
                 {"max_psl", std::to_string(max_psl)},
                 {"total_psl", std::to_string(total_psl)}};
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

      static constexpr forceinline size_t directory_address_count(const size_t& capacity) {
         return (capacity + BucketSize - 1) / BucketSize;
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
         std::array<Key, BucketSize> keys /*__attribute((aligned(sizeof(Key) * 8)))*/;
         std::array<Payload, BucketSize> payloads;
      } packed;

      std::vector<Bucket> slots;
   };

   template<class Key,
            class Payload,
            class HashFn,
            class ReductionFn,
            class ProbingFn,
            size_t BucketSize = 1,
            Key Sentinel = std::numeric_limits<Key>::max()>
   struct RobinhoodProbing {
     public:
      typedef Key KeyType;
      typedef Payload PayloadType;

     private:
      const HashFn hashfn;
      const ReductionFn reductionfn;
      const ProbingFn probingfn;
      const size_t capacity;
      bool allocated = false;

     public:
      explicit RobinhoodProbing(const size_t& capacity, const HashFn hashfn = HashFn())
         : hashfn(hashfn), reductionfn(ReductionFn(directory_address_count(capacity))),
           probingfn(ProbingFn(directory_address_count(capacity))), capacity(capacity){};

      RobinhoodProbing(RobinhoodProbing&&) = default;

      /**
       * Allocates hashtable memory. For production use, we would presumably call this in the constructor,
       * however this way we can save some time in case a measurement for this hashtable already exists.
       */
      void allocate() {
         if (allocated)
            return;
         allocated = true;

         // Reserve required memory
         slots.resize(directory_address_count(capacity));
         // Ensure all slots are in cleared state
         clear();
      }

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
      bool insert(const Key& k, const Payload& p) {
         // r+w variables (required to avoid insert recursion+issues)
         auto key = k;
         auto payload = p;

         const auto orig_key = key;
         const auto orig_payload = payload;

         if (unlikely(key == Sentinel)) {
            assert(false); // TODO: this must never happen in practice
            return false;
         }

         // Using template functor should successfully inline actual hash computation
         auto orig_slot_index = reductionfn(hashfn(key));
         auto slot_index = orig_slot_index;
         size_t probing_step = 0;

         for (;;) {
            auto& slot = slots[slot_index];
            for (size_t i = 0; i < BucketSize; i++) {
               if (slot.keys[i] == Sentinel) {
                  slot.keys[i] = key;
                  slot.payloads[i] = payload;
                  slot.psl[i] = probing_step;
                  return true;
               } else if (slot.keys[i] == key) {
                  // key already exists
                  return false;
               } else if (slot.psl[i] < probing_step) {
                  const auto rich_key = slot.keys[i];
                  const auto rich_payload = slot.payloads[i];
                  const auto rich_psl = slot.psl[i];

                  if (unlikely(orig_key == rich_key))
                     throw std::runtime_error("insertion failed, infinite loop detected");

                  slot.keys[i] = key;
                  slot.payloads[i] = payload;
                  slot.psl[i] = probing_step;

                  key = rich_key;
                  payload = rich_payload;
                  probing_step = rich_psl;

                  // This is important to guarantee lookup success, e.g.,
                  // for quadratic probing.
                  orig_slot_index = reductionfn(hashfn(key));
               }
            }

            // Slot is full, choose a new slot index based on probing function
            slot_index = probingfn(orig_slot_index, ++probing_step);
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
            slot_index = probingfn(orig_slot_index, ++probing_step);
            if (unlikely(slot_index == orig_slot_index))
               return std::nullopt;
         }
      }

      std::map<std::string, std::string> lookup_statistics(const std::vector<Key>& dataset) {
         size_t min_psl, max_psl, total_psl = 0;

         for (const auto& key : dataset) {
            // Using template functor should successfully inline actual hash computation
            const auto orig_slot_index = reductionfn(hashfn(key));
            auto slot_index = orig_slot_index;
            size_t probing_step = 0;

            for (;;) {
               auto& slot = slots[slot_index];
               for (size_t i = 0; i < BucketSize; i++) {
                  if (slot.keys[i] == key) {
                     min_psl = std::min(min_psl, probing_step);
                     max_psl = std::max(max_psl, probing_step);
                     total_psl += probing_step;
                     goto next;
                  }

                  if (slot.keys[i] == Sentinel)
                     goto next;
               }

               // Slot is full, choose a new slot index based on probing function
               slot_index = probingfn(orig_slot_index, ++probing_step);
               if (unlikely(slot_index == orig_slot_index))
                  goto next;
            }

         next:
            continue;
         }

         return {{"min_psl", std::to_string(min_psl)},
                 {"max_psl", std::to_string(max_psl)},
                 {"total_psl", std::to_string(total_psl)}};
      }

      static constexpr forceinline size_t bucket_byte_size() {
         return sizeof(Bucket);
      }

      static forceinline std::string name() {
         return ProbingFn::name() + "_robinhood_probing";
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
         return (capacity + BucketSize - 1) / BucketSize;
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

      ~RobinhoodProbing() {
         clear();
      }

     protected:
      struct Bucket {
         std::array<Key, BucketSize> keys /*__attribute((aligned(sizeof(Key) * 8)))*/;
         std::array<Payload, BucketSize> payloads;
         std::array<unsigned short, BucketSize> psl;
      } packed;

      std::vector<Bucket> slots;
   };
} // namespace Hashtable