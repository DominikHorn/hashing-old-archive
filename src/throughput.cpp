#include <iomanip>
#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"

// TODO: ensure that we don't use two separate reducers for 128bit hashes for throughput benchmark!
int main(const int argc, const char* argv[]) {
   std::ofstream outfile;

   try {
      auto args = Args::parse(argc, argv);
      outfile.open(args.outfile);
      outfile << "hash"
              << ",nanoseconds_total"
              << ",nanoseconds_per_key"
              << ",load_factor"
              << ",reducer"
              << ",dataset" << std::endl;

      // Prepare a tabulation hash table
      HASH_64 small_tabulation_table[0xFF] = {0};
      HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF] = {0};
      TabulationHash::gen_column(small_tabulation_table);
      TabulationHash::gen_table(large_tabulation_table);

      for (const auto& it : args.datasets) {
         std::cout << "dataset " << it.filename << std::endl;
         const auto dataset = it.load_shuffled(args.datapath);

         for (auto load_factor : args.load_factors) {
            const auto over_alloc = 1.0 / load_factor;
            const auto hashtable_size = static_cast<size_t>(static_cast<long double>(dataset.size()) * over_alloc);
            const auto magic_div = HashReduction::make_magic_divider(static_cast<HASH_64>(hashtable_size));
            const auto magic_branchfree_div =
               HashReduction::make_branchfree_magic_divider(static_cast<HASH_64>(hashtable_size));

            const auto measure_hashfn = [&](const std::string& method, const auto& hashfn) {
               const auto measure_hashfn_with_reducer = [&](const std::string& reducer, const auto& reducerfn) {
                  // Measure & log
                  std::cout << std::setw(55) << std::right << reducer + "(" + method + ") ... " << std::flush;
                  const auto stats = Benchmark::measure_throughput(dataset, over_alloc, hashfn, reducerfn);
                  std::cout << (static_cast<long double>(stats.total_inference_reduction_ns) /
                                static_cast<long double>(dataset.size()))
                            << "ns/key ("
                            << (static_cast<long double>(stats.total_inference_reduction_ns) / 1000000000.0)
                            << " s total)" << std::endl;

                  // Write to csv
                  outfile << method << "," << stats.total_inference_reduction_ns << ","
                          << (static_cast<long double>(stats.total_inference_reduction_ns) /
                              static_cast<long double>(dataset.size()))
                          << "," << load_factor << "," << reducer << "," << it.filename << std::endl;
               };

               measure_hashfn_with_reducer("do_nothing", HashReduction::do_nothing<HASH_64>);

               measure_hashfn_with_reducer("fastrange32", HashReduction::fastrange<HASH_32>);
               measure_hashfn_with_reducer("fastrange64", HashReduction::fastrange<HASH_64>);

               measure_hashfn_with_reducer("modulo", HashReduction::modulo<HASH_64>);
               measure_hashfn_with_reducer("fast_modulo", [&magic_div](const HASH_64& value, const HASH_64& n) {
                  return HashReduction::magic_modulo(value, n, magic_div);
               });
               measure_hashfn_with_reducer("branchless_fast_modulo",
                                           [&magic_branchfree_div](const HASH_64& value, const HASH_64& n) {
                                              return HashReduction::magic_modulo(value, n, magic_branchfree_div);
                                           });
            };

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

            // cold vs hot does not really make a difference, just benchmark not messing with the hardware prefetcher magic
            //            Cache::clearcache((void*) large_tabulation_table, sizeof(large_tabulation_table));
            //            Cache::prefetch_block<Cache::READ, Cache::HIGH>(&small_tabulation_table, sizeof(small_tabulation_table));
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
      outfile.close();
      return -1;
   }

   outfile.close();
   return 0;
}