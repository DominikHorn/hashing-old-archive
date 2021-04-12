#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/dataset.hpp"

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

            const auto measure = [&](const std::string& method, const auto& hashfn) {
               const auto log_and_write_results_csv = [&](const std::string& reducer,
                                                          const Benchmark::ThroughputStats& stats) {
                  std::cout << " took "
                            << (static_cast<long double>(stats.total_inference_reduction_ns) /
                                static_cast<long double>(dataset.size()))
                            << "ns per key (" << stats.total_inference_reduction_ns << " ns total)" << std::endl;
                  outfile << method << "," << stats.total_inference_reduction_ns << ","
                          << (static_cast<long double>(stats.total_inference_reduction_ns) /
                              static_cast<long double>(dataset.size()))
                          << "," << load_factor << "," << reducer << "," << it.filename << std::endl;
               };

               std::cout << "measuring do_nothing(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "do_nothing",
                  Benchmark::measure_throughput(dataset, over_alloc, hashfn, HashReduction::do_nothing<HASH_64>));

               std::cout << "measuring modulo(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "modulo",
                  Benchmark::measure_throughput(dataset, over_alloc, hashfn, HashReduction::modulo<HASH_64>));

               std::cout << "measuring fast_modulo(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "fast_modulo",
                  Benchmark::measure_throughput(dataset, over_alloc, hashfn,
                                                [&magic_div](const HASH_64& value, const HASH_64& n) {
                                                   return HashReduction::magic_modulo(value, n, magic_div);
                                                }));

               std::cout << "measuring fastrange(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "fastrange",
                  Benchmark::measure_throughput(dataset, over_alloc, hashfn, HashReduction::fastrange<HASH_64>));
            };

            // More significant bits supposedly are of higher quality for multiplicative methods -> compute
            // how much we need to shift to throw away as few "high quality" bits as possible
            const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);

            measure("mult64", [](HASH_64 key) { return MultHash::mult64_hash(key); });
            measure("mult64_shift", [p](HASH_64 key) { return MultHash::mult64_hash(key, p); });
            measure("fibo64", [](HASH_64 key) { return MultHash::fibonacci64_hash(key); });
            measure("fibo64_shift", [p](HASH_64 key) { return MultHash::fibonacci64_hash(key, p); });
            measure("fibo_prime64", [](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key); });
            measure("fibo_prime64_shift", [p](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key, p); });

            measure("multadd64", [](HASH_64 key) { return MultAddHash::multadd64_hash(key); });
            measure("multadd64_shift", [p](HASH_64 key) { return MultAddHash::multadd64_hash(key, p); });

            measure("murmur3_128_low",
                    [](HASH_64 key) { return HashReduction::lower_half(MurmurHash3::murmur3_128(key)); });
            measure("murmur3_128_upp",
                    [](HASH_64 key) { return HashReduction::upper_half(MurmurHash3::murmur3_128(key)); });
            measure("murmur3_128_xor",
                    [](HASH_64 key) { return HashReduction::xor_both(MurmurHash3::murmur3_128(key)); });
            measure("murmur3_128_city",
                    [](HASH_64 key) { return HashReduction::hash_128_to_64(MurmurHash3::murmur3_128(key)); });
            measure("murmur3_fin64", [](HASH_64 key) { return MurmurHash3::finalize_64(key); });

            measure("xxh64", [](HASH_64 key) { return XXHash::XXH64_hash(key); });
            measure("xxh3", [](HASH_64 key) { return XXHash::XXH3_hash(key); });
            measure("xxh3_128_low", [](HASH_64 key) { return HashReduction::lower_half(XXHash::XXH3_128_hash(key)); });
            measure("xxh3_128_upp", [](HASH_64 key) { return HashReduction::upper_half(XXHash::XXH3_128_hash(key)); });
            measure("xxh3_128_xor", [](HASH_64 key) { return HashReduction::xor_both(XXHash::XXH3_128_hash(key)); });
            measure("xxh3_128_city",
                    [](HASH_64 key) { return HashReduction::hash_128_to_64(XXHash::XXH3_128_hash(key)); });

            Cache::clearcache((void*) small_tabulation_table, sizeof(small_tabulation_table));
            measure("tabulation_small_cold64",
                    [&](HASH_64 key) { return TabulationHash::small_hash(key, small_tabulation_table); });
            Cache::clearcache((void*) large_tabulation_table, sizeof(large_tabulation_table));
            measure("tabulation_large_cold64",
                    [&](HASH_64 key) { return TabulationHash::large_hash(key, large_tabulation_table); });

            // tabulation_table should entirely be in cache due to the previous tabulation64 run; However, just to be sure:
            Cache::prefetch_block<Cache::READ, Cache::HIGH>(&small_tabulation_table, sizeof(small_tabulation_table));
            measure("tabulation_small_hot64",
                    [&](HASH_64 key) { return TabulationHash::small_hash(key, small_tabulation_table); });
            // tabulation_table should entirely be in cache due to the previous tabulation64 run; However, just to be sure:
            Cache::prefetch_block<Cache::READ, Cache::HIGH>(&large_tabulation_table, sizeof(large_tabulation_table));
            measure("tabulation_large_hot64",
                    [&](HASH_64 key) { return TabulationHash::large_hash(key, large_tabulation_table); });

            measure("city64", [](HASH_64 key) { return CityHash::CityHash64(key); });
            measure("city128_low", [](HASH_64 key) { return HashReduction::lower_half(CityHash::CityHash128(key)); });
            measure("city128_upp", [](HASH_64 key) { return HashReduction::upper_half(CityHash::CityHash128(key)); });
            measure("city128_xor", [](HASH_64 key) { return HashReduction::xor_both(CityHash::CityHash128(key)); });
            measure("city128_city",
                    [](HASH_64 key) { return HashReduction::hash_128_to_64(CityHash::CityHash128(key)); });

            measure("meow64_low", [](HASH_64 key) { return MeowHash::hash64(key); });
            measure("meow64_upp", [](HASH_64 key) { return MeowHash::hash64<1>(key); });
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