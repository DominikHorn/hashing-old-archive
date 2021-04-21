This repository aims to benchmark classical vs. learned hash functions. For this purpose, it contains various state of
the art implementations as well as benchmarking code.

For further information, see
our [collaborative google doc](https://docs.google.com/document/d/1akVt7XBPm3aWRnguZh88jpCAp97yZUwT8V5Po_p2Hxo/edit?usp=sharing)

# Development

## Repository layout

The repository is setup as a monorepo c++ project using CMake.

* `convenience/` contains an interface library comprised of convenience code
  (e.g., forceinline macros), used throughout the repository
* `data/` is meant to contain datasets. Also contains a python script for generating synthetic datasets and debug/test
  data. **NOTE: datasets should under no circumstances be uploaded to github (licensing, large file size)**. Real world
  datasets from our results may be found
  [here](https://dataverse.harvard.edu/dataset.xhtml?persistentId=doi:10.7910/DVN/JGVF9A)
* `hashing/` contains an interface library exposing various classical hash function implementations, optimized and tuned
  for small, fixed size keys
* `learned_models/` contains an interface library exposing learned models, prepared to be used as a replacement for
  classical hash functions
* `reduction/` contains an interface library implementing several methods for reducing hash values from [0, 2^p]
  to [0, N]
* `results/` contains benchmark results (csv) as well as plots and python code for generating said plots
* `src/` contains the actual benchmarking targets. Each target is implemented as a single .cpp file, linking against
  interface libraries from this repository aswell as shared convenience code found in `src/include`
* `thirdparty/` contains an interface library exposing third party libraries used by this project, e.g., cxxopts for
  parsing benchmark cli arguments

## Setup

### Cloning this repository

Either clone with submodules in one command:

```
git clone --recurse-submodules <repo-url>
```

Or clone regularily and then perform

```
git submodule update --init --recursive
```

### Running Benchmarks

All benchmarks are implemented as single ".cpp" executable targets, located in src/. To run them, compile the
corresponding target with cmake and execute the resulting binaries. To see the inline help text describing how to work
with the benchmarks, simply execute the binary without arguments or with "-h" or with "--help".

Alternatively you may use the `build.sh` or `benchmark.sh` scripts. The latter will execute `build.sh` automatically.

# Results

See the `results/` folder or, more specifically, the folders contained therein.