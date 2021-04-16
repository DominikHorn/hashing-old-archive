#define VERBOSE

#include <iomanip>
#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/random_hash.hpp"

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);
      CSV outfile(args.outfile,
                  {
                     "dataset",
                     "numelements",
                     "load_factor",
                     "hash",
                     "reducer",
                     "min",
                     "max",
                     "std_dev",
                     "empty_slots",
                     "empty_slots_percent",
                     "colliding_slots",
                     "colliding_slots_percent",
                     "total_colliding_keys",
                     "total_colliding_keys_percent",
                     "nanoseconds_total",
                     "nanoseconds_per_key",
                  });

      // Precompute tabulation hash tables
      HASH_64 small_tabulation_table[0xFF] = {0};
      HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF] = {0};
      TabulationHash::gen_column(small_tabulation_table);
      TabulationHash::gen_table(large_tabulation_table);

      for (const auto& it : args.datasets) {
         const auto dataset = it.load(args.datapath);

         // TODO: Build dataset specific auxiliary data (e.g., pgm, rmi)
         // TODO: introduce multithreading i.e., one thread per load factor!

         for (auto load_factor : args.load_factors) {
            const auto over_alloc = 1.0 / load_factor;
            const auto hashtable_size =
               static_cast<uint64_t>(static_cast<double>(dataset.size()) * static_cast<double>(over_alloc));
            std::vector<uint32_t> collision_counter(hashtable_size);

            // Build load factor specific auxiliary data
            const auto magic_branchfree_div =
               HashReduction::make_branchfree_magic_divider(static_cast<HASH_64>(hashtable_size));

            // measures a given hash function with a given reducer
            const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const auto& hashfn,
                                                         const std::string& reducer_name, const auto& reducerfn) {
            // Measure & log
#ifdef VERBOSE
               std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") ... " << std::flush;
#endif
               std::fill(collision_counter.begin(), collision_counter.end(), 0);
               const auto stats = Benchmark::measure_collisions(dataset, collision_counter, hashfn, reducerfn);
#ifdef VERBOSE
               std::cout << relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size()) << " ns/key ("
                         << nanoseconds_to_seconds(stats.inference_reduction_memaccess_total_ns) << " s total)"
                         << std::endl;
#endif

               const auto str = [](auto s) { return std::to_string(s); };
               outfile.write({
                  {"dataset", it.filename},
                  {"numelements", str(dataset.size())},
                  {"load_factor", str(load_factor)},
                  {"hash", hash_name},
                  {"reducer", reducer_name},
                  {"min", str(stats.min)},
                  {"max", str(stats.max)},
                  {"std_dev", str(stats.std_dev)},
                  {"empty_buckets", str(stats.empty_buckets)},
                  {"empty_buckets_percent", str(relative_to(stats.empty_buckets, hashtable_size))},
                  {"colliding_buckets", str(stats.colliding_buckets)},
                  {"colliding_buckets_percent", str(relative_to(stats.colliding_buckets, hashtable_size))},
                  {"total_colliding_keys", str(stats.total_colliding_keys)},
                  {"total_colliding_keys_percent", str(relative_to(stats.total_colliding_keys, hashtable_size))},
                  {"nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns)},
                  {"nanoseconds_per_key",
                   str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size()))},
               });
            };

            // measures a hash function using every reducer
            const auto measure_hashfn = [&](const std::string& hash_name, const auto& hashfn) {
               measure_hashfn_with_reducer(hash_name, hashfn, "fastrange32", HashReduction::fastrange<HASH_32>);
               measure_hashfn_with_reducer(hash_name, hashfn, "fastrange64", HashReduction::fastrange<HASH_64>);

               // modulo, fast_modulo and branchless_fast_modulo only differ in the speed at which they complete computations
               measure_hashfn_with_reducer(hash_name, hashfn, "branchless_fast_modulo",
                                           [&magic_branchfree_div](const HASH_64& value, const HASH_64& n) {
                                              return HashReduction::magic_modulo(value, n, magic_branchfree_div);
                                           });
            };

            /**
             * ====================================
             *           Actual measuring
             * ====================================
             */

            // Uniform random baseline(random prime constant seed)
            RandomHash rhash(hashtable_size);
            measure_hashfn_with_reducer(
               "uniform_random",
               [&](HASH_64 key) {
                  UNUSED(key);
                  return rhash.next();
               },
               "do_nothing", HashReduction::do_nothing<HASH_64>);

            measure_hashfn("mult64", [](HASH_64 key) { return MultHash::mult64_hash(key); });
            measure_hashfn("fibo64", [](HASH_64 key) { return MultHash::fibonacci64_hash(key); });
            measure_hashfn("fibo_prime64", [](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key); });
            measure_hashfn("multadd64", [](HASH_64 key) { return MultAddHash::multadd64_hash(key); });

            // More significant bits supposedly are of higher quality for multiplicative methods -> compute
            // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
            const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);
            measure_hashfn("mult64_shift" + std::to_string(p),
                           [p](HASH_64 key) { return MultHash::mult64_hash(key, p); });
            measure_hashfn("fibo64_shift" + std::to_string(p),
                           [p](HASH_64 key) { return MultHash::fibonacci64_hash(key, p); });
            measure_hashfn("fibo_prime64_shift" + std::to_string(p),
                           [p](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key, p); });
            measure_hashfn("multadd64_shift" + std::to_string(p),
                           [p](HASH_64 key) { return MultAddHash::multadd64_hash(key, p); });
            const unsigned int rot = 64 - p;
            measure_hashfn("mult64_rotate" + std::to_string(rot),
                           [&](HASH_64 key) { return rotr(MultHash::mult64_hash(key), rot); });
            measure_hashfn("fibo64_rotate" + std::to_string(rot),
                           [&](HASH_64 key) { return rotr(MultHash::fibonacci64_hash(key), rot); });
            measure_hashfn("fibo_prime64_rotate" + std::to_string(rot),
                           [&](HASH_64 key) { return rotr(MultHash::fibonacci_prime64_hash(key), rot); });
            measure_hashfn("multadd64_rotate" + std::to_string(rot),
                           [&](HASH_64 key) { return rotr(MultAddHash::multadd64_hash(key), rot); });

            measure_hashfn("murmur3_128_low",
                           [](HASH_64 key) { return HashReduction::lower_half(MurmurHash3::murmur3_128(key)); });
            measure_hashfn("murmur3_128_upp",
                           [](HASH_64 key) { return HashReduction::upper_half(MurmurHash3::murmur3_128(key)); });
            measure_hashfn("murmur3_128_xor",
                           [](HASH_64 key) { return HashReduction::xor_both(MurmurHash3::murmur3_128(key)); });
            measure_hashfn("murmur3_128_city",
                           [](HASH_64 key) { return HashReduction::hash_128_to_64(MurmurHash3::murmur3_128(key)); });
            measure_hashfn("murmur3_fin64", [](HASH_64 key) { return MurmurHash3::finalize_64(key); });

            measure_hashfn("xxh64", [](HASH_64 key) { return XXHash::XXH64_hash(key); });
            measure_hashfn("xxh3", [](HASH_64 key) { return XXHash::XXH3_hash(key); });
            measure_hashfn("xxh3_128_low",
                           [](HASH_64 key) { return HashReduction::lower_half(XXHash::XXH3_128_hash(key)); });
            measure_hashfn("xxh3_128_upp",
                           [](HASH_64 key) { return HashReduction::upper_half(XXHash::XXH3_128_hash(key)); });
            measure_hashfn("xxh3_128_xor",
                           [](HASH_64 key) { return HashReduction::xor_both(XXHash::XXH3_128_hash(key)); });
            measure_hashfn("xxh3_128_city",
                           [](HASH_64 key) { return HashReduction::hash_128_to_64(XXHash::XXH3_128_hash(key)); });

            measure_hashfn("tabulation_small64",
                           [&](HASH_64 key) { return TabulationHash::small_hash(key, small_tabulation_table); });
            measure_hashfn("tabulation_large64",
                           [&](HASH_64 key) { return TabulationHash::large_hash(key, large_tabulation_table); });

            measure_hashfn("city64", [](HASH_64 key) { return CityHash::CityHash64(key); });
            measure_hashfn("city128_low",
                           [](HASH_64 key) { return HashReduction::lower_half(CityHash::CityHash128(key)); });
            measure_hashfn("city128_upp",
                           [](HASH_64 key) { return HashReduction::upper_half(CityHash::CityHash128(key)); });
            measure_hashfn("city128_xor",
                           [](HASH_64 key) { return HashReduction::xor_both(CityHash::CityHash128(key)); });
            measure_hashfn("city128_city",
                           [](HASH_64 key) { return HashReduction::hash_128_to_64(CityHash::CityHash128(key)); });

            measure_hashfn("meow64_low", [](HASH_64 key) { return MeowHash::hash64(key); });
            measure_hashfn("meow64_upp", [](HASH_64 key) { return MeowHash::hash64<1>(key); });

            measure_hashfn("aqua_low", [](HASH_64 key) { return AquaHash::hash64(key); });
            measure_hashfn("aqua_upp", [](HASH_64 key) { return AquaHash::hash64<1>(key); });
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}