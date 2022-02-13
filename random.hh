/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

/*
 * This code is derived from the full C implementation, which is in turn
 * derived from the canonical C++ PCG implementation. The C++ version
 * has many additional features and is preferable if you can use C++ in
 * your project.
 */
#include <inttypes.h>

struct pcg_state_setseq_64 {    // Internals are *Private*.
    uint64_t state;             // RNG state.  All values are possible.
    uint64_t inc;               // Controls which RNG sequence (stream) is
                                // selected. Must *always* be odd.
};
using PCG = pcg_state_setseq_64;

// If you *must* statically initialize it, here's one.

constexpr PCG PCG_INITIALIZER = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };

PCG pcg_global = PCG_INITIALIZER;

// pcg_random32()
// pcg_random32(rng)
//     Generate a uniformly distributed 32-bit random number
uint32_t pcg_random32(PCG* rng = &pcg_global) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (uint32_t)((xorshifted >> rot) | (xorshifted << (((uint32_t)(-(int32_t)rot)) & 31)));
}

//     Generate a uniformly distributed 64-bit random number
inline uint64_t pcg_random64(PCG* rng = &pcg_global) {
	uint64_t r0 = pcg_random32(rng);
	uint64_t r1 = pcg_random32(rng);
	return (r1<<32)|r0;
}

// pcg_seed(initstate, initseq)
// pcg_seed(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)
void pcg_seed(PCG* rng, uint64_t initstate, uint64_t sequence) {
    rng->state = 0U;
    rng->inc = (sequence << 1u) | 1u;
    pcg_random32(rng);
    rng->state += initstate;
    pcg_random32(rng);
}
inline void pcg_seed(uint64_t initstate, uint64_t seq) {
	pcg_seed(&pcg_global, initstate, seq);
}
inline void pcg_seed(PCG* rng, uint64_t seed) {
    pcg_seed(rng, seed, seed);
}
inline void pcg_seed(uint64_t seed) {
    pcg_seed(&pcg_global, seed);
}


inline uint64_t _pcg_advance_lcg_64(uint64_t state, uint64_t delta, uint64_t cur_mult, uint64_t cur_plus) {
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    while (delta > 0) {
        if (delta & 1) {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta /= 2;
    }
    return acc_mult * state + acc_plus;
}

inline void pcg_advance(PCG* rng, uint64_t delta) {
	constexpr uint64_t PCG_DEFAULT_MULTIPLIER_64 = 6364136223846793005ULL;
    rng->state = _pcg_advance_lcg_64(rng->state, delta, PCG_DEFAULT_MULTIPLIER_64, rng->inc);
}



// pcg_random_bound(bound):
// pcg_random_bound(rng, bound):
//     Generate a uniformly distributed number, r, where lower <= r <= upper
inline uint32_t pcg_random32_in(PCG* rng, uint32_t lower, uint32_t upper) {
	uint32_t bound = upper - lower + 1;
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = ((uint32_t)(-(int32_t)bound)) % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg_random32(rng);
        if (r >= threshold)
            return (r % bound) + lower;
    }
}
inline uint32_t pcg_random32_in(uint32_t lower, uint32_t upper) {
    return pcg_random32_in(&pcg_global, lower, upper);
}

// pcg_random_uniform(bound):
// pcg_random_uniform(rng, bound):
//     Generate a uniformly distributed number, r in [0, 1)
inline float pcg_random_uniform(PCG* rng = &pcg_global) {
	constexpr float NEG_32 = 1/(((float)~0u) + 1);
	uint32_t r = pcg_random32(rng);
	return r*NEG_32;
}
//     Generate a uniformly distributed number, r in [0, 1]
inline float pcg_random_uniform_in(PCG* rng = &pcg_global) {
	uint32_t r = pcg_random32(rng);
	return ((float)r)/((float)~0u);
}
//     Generate a uniformly distributed number, r in (0, 1)
inline float pcg_random_uniform_ex(PCG* rng = &pcg_global) {
	uint32_t r = pcg_random32(rng);
	return (((float)r) + 1)/(((float)~0u) + 2);
}
