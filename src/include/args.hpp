#ifndef ARGS_H
#define ARGS_H

#include <map>
#include <string>
#include <vector>

/**
 * Dataset to perform the benchmark on
 */
struct Dataset {
   // TODO: add dataset data that we will need outside of args parsing
};

/**
 * Parsed command line args of the application
 */
struct Args {
   // TODO: ensure args has everything we actually need for benchmark execution
   Dataset dataset;
   double over_alloc;
   int bucket_size;
};

/**
 * Parses the given set of command line args
 */
Args parse(int argc, char* argv[]);

#endif