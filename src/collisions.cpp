#include <iomanip>
#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/random_hash.hpp"

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
      HASH_64 small_tabulation_table[0xFF] = {0};
      HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF] = {0};
      TabulationHash::gen_column(small_tabulation_table);
      TabulationHash::gen_table(large_tabulation_table);

      for (const auto& it : args.datasets) {
         std::cout << "dataset " << it.filename << std::endl;
         const auto dataset = it.load(args.datapath);

         for (auto load_factor : args.load_factors) {
            const auto over_alloc = 1.0 / load_factor;
            const auto hashtable_size = static_cast<uint64_t>(static_cast<long double>(dataset.size()) * over_alloc);
            const auto magic_div = HashReduction::make_magic_divider(static_cast<HASH_64>(hashtable_size));
            const auto magic_branchfree_div =
               HashReduction::make_branchfree_magic_divider(static_cast<HASH_64>(hashtable_size));

            const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const auto& hashfn,
                                                         const std::string& reducer_name, const auto& reducerfn) {
               // Measure & log
               std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") ... " << std::flush;
               const auto stats = Benchmark::measure_collisions(dataset, over_alloc, hashfn, reducerfn);
               std::cout << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                             static_cast<long double>(dataset.size()))
                         << "ns/key ("
                         << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                             static_cast<long double>(1000000000.0))
                         << " s total)" << std::endl;

               // Write to csv
               outfile << hash_name << "," << stats.min << "," << stats.max << "," << stats.std_dev << ","
                       << stats.empty_buckets << "," << stats.colliding_buckets << "," << stats.total_collisions << ","
                       << stats.inference_reduction_memaccess_total_ns << ","
                       << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                           static_cast<long double>(dataset.size()))
                       << "," << load_factor << "," << reducer_name << "," << it.filename << std::endl;
            };

            const auto measure_hashfn = [&](const std::string& hash_name, auto hashfn) {
               measure_hashfn_with_reducer(hash_name, hashfn, "fastrange", HashReduction::fastrange<HASH_64>);

               // TODO: disable the following two for benchmark (Only required to verify branchless fast modulo)
               measure_hashfn_with_reducer(hash_name, hashfn, "modulo", HashReduction::modulo<HASH_64>);
               measure_hashfn_with_reducer(hash_name, hashfn, "fast_modulo",
                                           [&magic_div](const HASH_64& value, const HASH_64& n) {
                                              return HashReduction::magic_modulo(value, n, magic_div);
                                           });

               measure_hashfn_with_reducer(hash_name, hashfn, "branchless_fast_modulo",
                                           [&magic_branchfree_div](const HASH_64& value, const HASH_64& n) {
                                              return HashReduction::magic_modulo(value, n, magic_branchfree_div);
                                           });
            };

            // More significant bits supposedly are of higher quality for multiplicative methods -> compute
            // how much we need to shift to throw away as few "high quality" bits as possible
            const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);

            // Uniform random baseline (random prime constant)
            RandomHash rhash(hashtable_size);
            measure_hashfn_with_reducer(
               "uniform_random", [&](HASH_64 key) { return rhash.next(); }, "do_nothing",
               HashReduction::do_nothing<HASH_64>);

            measure_hashfn("mult64", [](HASH_64 key) { return MultHash::mult64_hash(key); });
            measure_hashfn("mult64_shift", [p](HASH_64 key) { return MultHash::mult64_hash(key, p); });
            measure_hashfn("fibo64", [](HASH_64 key) { return MultHash::fibonacci64_hash(key); });
            measure_hashfn("fibo64_shift", [p](HASH_64 key) { return MultHash::fibonacci64_hash(key, p); });
            measure_hashfn("fibo_prime64", [](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key); });
            measure_hashfn("fibo_prime64_shift", [p](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key, p); });
            measure_hashfn("multadd64", [](HASH_64 key) { return MultAddHash::multadd64_hash(key); });
            measure_hashfn("multadd64_shift", [p](HASH_64 key) { return MultAddHash::multadd64_hash(key, p); });
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