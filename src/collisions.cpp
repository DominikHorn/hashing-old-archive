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
              << ",empty_buckets_percent"
              << ",colliding_buckets"
              << ",colliding_buckets_percent"
              << ",total_colliding_keys"
              << ",total_colliding_keys_percent"
              << ",nanoseconds_total"
              << ",nanoseconds_per_key"
              << ",load_factor"
              << ",reducer"
              << ",dataset"
              << ",numelements" << std::endl;

      // Prepare a tabulation hash table
      HASH_64 small_tabulation_table[0xFF] = {0};
      HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF] = {0};
      TabulationHash::gen_column(small_tabulation_table);
      TabulationHash::gen_table(large_tabulation_table);

      for (const auto& it : args.datasets) {
         std::cout << "dataset " << it.filename << std::endl;
         const auto dataset = it.load_shuffled(args.datapath);

         // TODO: introduce multithreading i.e., one thread per load factor!
         for (auto load_factor : args.load_factors) {
            const auto over_alloc = 1.0 / load_factor;
            const auto hashtable_size =
               static_cast<uint64_t>(static_cast<double>(dataset.size()) * static_cast<double>(over_alloc));
            std::vector<uint32_t> collision_counter(hashtable_size);

            const auto magic_branchfree_div =
               HashReduction::make_branchfree_magic_divider(static_cast<HASH_64>(hashtable_size));

            const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const auto& hashfn,
                                                         const std::string& reducer_name, const auto& reducerfn) {
               // Measure & log
               std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") ... " << std::flush;
               const auto stats = Benchmark::measure_collisions(dataset, collision_counter, hashfn, reducerfn);
               std::cout << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                             static_cast<long double>(dataset.size()))
                         << " ns/key ("
                         << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                             static_cast<long double>(1000000000.0))
                         << " s total)" << std::endl;

               // Write to csv
               outfile << hash_name << "," // hash function name
                       << stats.min << "," // min keys found in a single bucket after hashing (should be 0)
                       << stats.max << "," // max keys found in a single bucket (must be > 0)
                       << stats.std_dev << "," // std_dev regarding amount of keys per bucket
                       << stats.empty_buckets << "," // absolute amount of empty buckets
                       << static_cast<double>(stats.empty_buckets) / static_cast<double>(hashtable_size)
                       << "," // relative amount of empty buckets
                       << stats.colliding_buckets << ","
                       << static_cast<double>(stats.colliding_buckets) / static_cast<double>(hashtable_size)
                       << "," // relative amount of buckets with collisions, i.e., buckets with more than 1 element
                       << stats.total_colliding_keys << "," // total amount of colliding keys
                       << static_cast<double>(stats.total_colliding_keys) / static_cast<double>(dataset.size())
                       << "," // relative amount of colliding keys
                       << stats.inference_reduction_memaccess_total_ns << ","
                       << (static_cast<long double>(stats.inference_reduction_memaccess_total_ns) /
                           static_cast<long double>(dataset.size()))
                       << "," << load_factor << "," // load factor of the hashtable
                       << reducer_name << "," // name of the reducer
                       << it.filename << "," // dataset name
                       << dataset.size() // amount of keys in the dataset
                       << std::endl;
            };

            const auto measure_hashfn = [&](const std::string& hash_name, auto hashfn) {
               measure_hashfn_with_reducer(hash_name, hashfn, "fastrange32", HashReduction::fastrange<HASH_32>);
               measure_hashfn_with_reducer(hash_name, hashfn, "fastrange64", HashReduction::fastrange<HASH_64>);

               // modulo, fast_modulo and branchless_fast_modulo only differ in the speed at which they complete computations
               measure_hashfn_with_reducer(hash_name, hashfn, "branchless_fast_modulo",
                                           [&magic_branchfree_div](const HASH_64& value, const HASH_64& n) {
                                              return HashReduction::magic_modulo(value, n, magic_branchfree_div);
                                           });
            };

            // Uniform random baseline(random prime constant)
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
      outfile.close();
      return -1;
   }

   outfile.close();
   return 0;
}