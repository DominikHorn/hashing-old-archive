#pragma once

/**
 * This file simply reexports other file's definitions, i.e.,
 * is meant as the single import source for all relevant
 * classical hash functions
 */

#include "reduction.hpp"
#include "wrappers/city.hpp"
#include "wrappers/mult.hpp"
#include "wrappers/multadd.hpp"
#include "wrappers/murmur.hpp"
#include "wrappers/tabulation.hpp"
#include "wrappers/types.hpp"
#include "wrappers/xxh.hpp"