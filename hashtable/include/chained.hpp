#pragma once

#include <array>
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

     public:
      explicit Chained(const size_t& size, const ReductionFn& reductionfn = ReductionFn(),
                       const HashFn& hashfn = HashFn())
         : hashfn(hashfn), reductionfn(reductionfn), slots(size) {
         for (auto& slot : slots) {
            slot.keys.fill(Sentinel);
         }
      };

      Chained(Chained&&) = default;

      bool insert(const Key& key, const Payload payload) {
         // Using template functor should successfully inline actual hash computation
         const auto slot_index = reductionfn(hashfn(key), this->directory_size());

         Bucket* slot = &slots[slot_index];
         for (;;) {
            // Find suitable empty entry place. Note that deletions with holes will require
            // searching entire bucket to deal with duplicate keys!
            for (size_t i = 0; i < BucketSize; i++) {
               if (slot->keys[i] == Sentinel) {
                  slot->keys[i] = key;
                  slot->payloads[i] = payload;
                  return true;
               } else if (slot->keys[i] == key) {
                  // entry already exists
                  return false;
               }
            }

            if (slot->next == nullptr)
               break;

            slot = slot->next;
         }

         // Append a new bucket to the chain and add element there
         auto b = new Bucket();
         b->keys[0] = key;
         b->payloads[0] = payload;
         slot->next = b;
         return true;
      }

      std::optional<Payload> lookup(const Key& key) const {
         // Using template functor should successfully inline actual hash computation
         const auto slot_index = reductionfn(hashfn(key), this->directory_size());

         auto slot = &slots[slot_index];

         while (slot != nullptr) {
            for (size_t i = 0; i < BucketSize; i++) {
               if (slot->keys[i] == key)
                  return std::make_optional(slot->payloads[i]);

               if (slot->keys[i] == Sentinel)
                  return std::nullopt;
            }
            slot = slot->next;
         }

         return std::nullopt;
      }

      static constexpr forceinline size_t bucket_byte_size() {
         return sizeof(Bucket);
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

      void clear() {
         for (auto& slot : slots) {
            for (size_t i = 0; i < BucketSize; i++) {
               slot.keys[i] = Sentinel;
            }
            auto current = slot.next;
            slot.next = nullptr;

            while (current != nullptr) {
               auto next = current->next;
               delete current;
               current = next;
            }
         }
      }

      ~Chained() {
         clear();
      }

     protected:
      struct Bucket {
         std::array<Key, BucketSize> keys __attribute((aligned(sizeof(Key) * 8))) = {};
         std::array<Payload, BucketSize> payloads = {};
         Bucket* next = nullptr;
      } packed;

      // First bucket is always inline in the slot
      std::vector<Bucket> slots;

      forceinline size_t directory_size() const {
         return slots.size();
      }
   };
} // namespace Hashtable
