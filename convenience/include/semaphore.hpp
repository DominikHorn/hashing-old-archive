#pragma once

#include <condition_variable>
#include <mutex>

#include "builtins.hpp"

namespace std {
   /**
    * Custom counting_semaphore. Replace as soon as c++20 counting_semaphore implementation is available
    * (I could not find the appropriate header and since the implementation required by our code
    * is rather trivial I decided to quickly whip this together)
    *
    * @tparam max_value
    */
   struct counting_semaphore {
      counting_semaphore(unsigned int max_value) : counter(max_value), max_value(max_value) {}

      counting_semaphore(unsigned int max_value, unsigned int start_cnt) : counter(start_cnt), max_value(max_value) {}

      /**
       * Atomically increments the internal counter by the value of update.
       * Any thread(s) waiting for the counter to be greater than `0`,
       * such as due to being blocked in acquire, will subsequently be unblocked.
       */
      forceinline void release(const unsigned int& update = 1) {
         std::unique_lock<std::mutex> lock(mutex);

         if (unlikely(update > max_value - counter)) {
            throw std::runtime_error("Can not increase semaphore value by more than max_value - counter");
         }

         counter += update;
         for (auto i = update; i > 0; i--)
            condition_variable.notify_one();
      }

      /**
       * Atomically decrements the internal counter by 1 if it is greater than `0`; otherwise blocks
       * until it is greater than `0` and can successfully decrement the internal counter.
       */
      forceinline void aquire() {
         std::unique_lock<std::mutex> lock(mutex);
         while (counter == 0) {
            condition_variable.wait(lock);
         }
         counter--;
      }

     private:
      std::condition_variable condition_variable;
      std::mutex mutex;
      unsigned int counter;
      const unsigned int max_value;
   };
} // namespace std