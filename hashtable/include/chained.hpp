#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <convenience.hpp>

namespace Hashtable {
   template<typename Key, typename Payload, size_t BucketSize = 1>
   struct Chained {
      Chained(const size_t size) : slots(size){};

      // TODO: insert is not properly inlined at callsite by clang (gcc untested)
      template<typename Hash>
      forceinline bool insert(const Key& key, const Payload payload, const Hash& hashfn) {
         // Using template functor should successfully inline actual hash computation
         const auto slot_index = hashfn(key, this->size());

         Bucket* next_slot = &slots[slot_index];
         Bucket* slot = nullptr;

         while (next_slot != nullptr) {
            slot = next_slot;
            next_slot = slot->next;

            // Find suitable empty entry place. Note that deletions with holes will require
            // searching entire bucket to deal with duplicate keys!
            for (size_t i = 0; i < BucketSize; i++) {
               if (!slot->entries[i].has_value) {
                  slot->entries[i].set(key, payload);
                  return true;
               } else if (slot->entries[i].key == key) {
                  // entry already exists
                  return false;
               }
            }
         }

         // Append a new bucket to the chain and add element there
         auto b = new Bucket();
         b->entries[0].set(key, payload);
         slot->next = b;
         return true;
      }

      // TODO: lookup is not properly inlined at callsite by clang (gcc untested)
      template<typename Hash>
      forceinline std::optional<Payload> lookup(const Key& key, const Hash& hashfn) const {
         // Using template functor should successfully inline actual hash computation
         const auto slot_index = hashfn(key, this->size());

         auto slot = &slots[slot_index];

         while (slot != nullptr) {
            for (size_t i = 0; i < BucketSize; i++) {
               const auto entry = slot->entries[i];
               if (!entry.has_value) {
                  return std::nullopt;
               } else if (entry.key == key) {
                  return {entry.key};
               }
            }
            slot = slot->next;
         }

         return std::nullopt;
      }

      forceinline size_t size() const {
         return slots.size();
      }

      static constexpr forceinline size_t bucket_size() {
         return sizeof(Bucket);
      }

      void clear() {
         for (auto& slot : slots) {
            for (size_t i = 0; i < BucketSize; i++) {
               slot.entries[i].clear();
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
         for (auto& slot : slots) {
            auto current = slot.next;
            while (current != nullptr) {
               auto next = current->next;
               delete current;
               current = next;
            }
         }
      }

     protected:
      struct Bucket {
         struct Entry {
            Key key = 0x0;
            Payload payload = 0x0;
            bool has_value = false;

            forceinline void set(const Key& key, const Payload& payload) {
               this->key = key;
               this->payload = payload;
               this->has_value = true;
            }

            forceinline void clear() {
               this->has_value = false;
            }
         } packed;

         std::array<Entry, BucketSize> entries;
         Bucket* next = nullptr;
      } packed;

      // First bucket is always inline in the slot
      std::vector<Bucket> slots;
   };

} // namespace Hashtable
