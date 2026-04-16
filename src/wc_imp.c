#define WC_IMPLEMENTATION

#include "common_single.h"

#include "gen_vector_single.h"

#include "String_single.h"

// alignment messes up our alloc calculation
#define ARENA_DEFAULT_ALIGNMENT 0
#include "arena_single.h"   

#include "random_single.h"
