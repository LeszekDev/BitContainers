#include <cstdint>

class FastRandom {
private:
    uint64_t state;

public:
    // Seed it with any value (e.g., time or a constant)
    FastRandom(uint64_t seed = 88) : state(seed) {}

    // Returns a pseudo-random number between 0 and 2^32 - 1
    inline uint32_t next() {
        state = (6364136223846793005ULL * state + 1442695040888963407ULL);
        return (uint32_t)(state >> 32);
    }
};