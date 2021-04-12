#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"

int main(const int argc, const char* argv[]) {
   std::ofstream outfile;

   try {
      auto args = Args::parse(argc, argv);
      outfile.open(args.outfile);
      outfile << "hash"
              << ",min"
              << ",max"
              << ",std_dev"
              << ",empty_buckets"
              << ",colliding_buckets"
              << ",total_collisions"
              << ",nanoseconds_total"
              << ",nanoseconds_per_key"
              << ",load_factor"
              << ",reducer"
              << ",dataset" << std::endl;

      // Prepare a tabulation hash table
      HASH_64 tabulation_table[sizeof(HASH_64)][0xFF] = {0};
      TabulationHash::gen_table(tabulation_table);

      for (const auto& it : args.datasets) {
         std::cout << "dataset " << it.filename << std::endl;
         const auto dataset = it.load(args.datapath);

         for (auto load_factor : args.load_factors) {
            const auto over_alloc = 1.0 / load_factor;
            const auto hashtable_size = static_cast<size_t>(static_cast<long double>(dataset.size()) * over_alloc);
            const auto magic_div = HashReduction::make_magic_divider(static_cast<HASH_64>(hashtable_size));

            const auto measure = [&](const std::string& method, auto hashfn) {
               const auto log_and_write_results_csv = [&](const std::string& reducer,
                                                          const Benchmark::CollisionStats<uint64_t, double>& stats) {
                  std::cout << " took " << (stats.inference_reduction_memaccess_total_ns / dataset.size())
                            << "ns per key (" << stats.inference_reduction_memaccess_total_ns << " ns total)"
                            << std::endl;
                  outfile << method << "," << stats.min << "," << stats.max << "," << stats.std_dev << ","
                          << stats.empty_buckets << "," << stats.colliding_buckets << "," << stats.total_collisions
                          << "," << stats.inference_reduction_memaccess_total_ns << ","
                          << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                              static_cast<long double>(dataset.size()))
                          << "," << load_factor << "," << reducer << "," << it.filename << std::endl;
               };

               // regular modulo collision measurement is only necessary to ensure that fast_modulo is implemented correctly
               std::cout << "measuring modulo(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "modulo",
                  Benchmark::measure_collisions(dataset, over_alloc, hashfn, HashReduction::modulo<HASH_64>));

               std::cout << "measuring fast_modulo(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "fast_modulo",
                  Benchmark::measure_collisions(dataset,
                                                over_alloc,
                                                hashfn,
                                                [&magic_div](const HASH_64& value, const HASH_64& n) {
                                                   return HashReduction::magic_modulo(value, n, magic_div);
                                                }));

               std::cout << "measuring fastrange(" << method << ") ..." << std::flush;
               log_and_write_results_csv(
                  "fastrange",
                  Benchmark::measure_collisions(dataset, over_alloc, hashfn, HashReduction::fastrange<HASH_64>));
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
            measure("tabulation64", [&](HASH_64 key) { return TabulationHash::naive_hash(key, tabulation_table); });
            measure("city64", [](HASH_64 key) { return CityHash::CityHash64(key); });
            measure("city128_low", [](HASH_64 key) { return HashReduction::lower_half(CityHash::CityHash128(key)); });
            measure("city128_upp", [](HASH_64 key) { return HashReduction::upper_half(CityHash::CityHash128(key)); });
            measure("city128_xor", [](HASH_64 key) { return HashReduction::xor_both(CityHash::CityHash128(key)); });
            measure("city128_city",
                    [](HASH_64 key) { return HashReduction::hash_128_to_64(CityHash::CityHash128(key)); });
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