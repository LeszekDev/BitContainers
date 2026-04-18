![BitVector](images/bitvector-banner.png)

# BitVector

**Version:** 1.0.4  
**Author:** @LeszekDev  
**License:** Apache 2.0  
**Requires:** C++11 (Recommended C++20/C++23)  
*(Note: Even though it technically works with C++11, it's not officially supported)*


BitVector is a lightweight, single-header C++ library providing a highly optimized container for bit-packed data. Designed with a familiar `std::vector`-style interface, BitVector limits operations to a maximum width of 64 bits (`uint64_t`) per operation to minimize the CPU overhead traditionally associated with dynamic-width bit packing. 

The primary architectural goal of BitVector is **spatial efficiency**. It is intended for environments where memory footprint, disk storage, or network bandwidth is the primary bottleneck, rather than raw memory access.

## Key Features

* **Single-Header Design:** Easy integration into any build system.
* **Deterministic Endianness:** Memory layout is strictly enforced as Little-Endian. The library automatically utilizes `std::byteswap` (C++23), `std::endian` (C++20), compiler intrinsics, or manual bit-shifting to ensure consistent data serialization across different architectures.
* **Boundary Stitching:** Safely reads and writes bit sequences that cross the internal 64-bit block boundaries.
* **Direct Buffer Access:** Exposes the underlying `std::vector<uint64_t>` for rapid bulk operations, serialization, or network transmission.

## Ideal Use Cases

* **Network Protocol Serialization:** Packing custom-width game state data (e.g., 7-bit health, 11-bit rotation) before transmitting over TCP/UDP.
* **Memory-Constrained Environments:** Storing massive arrays of tightly packed boolean or low-range integer states.
* **Disk Compression:** Pre-packing data structures to minimize I/O overhead.

## Integration

BitVector is a single-header library. To use it, include `BitVector.hpp` in your files. 

In **exactly one** `.cpp` file, define `LESZEK_BITVECTOR_IMPLEMENTATION` before including the header to compile the implementation.

```cpp
// BitVector.cpp
#define LESZEK_BITVECTOR_IMPLEMENTATION
#include "BitVector.hpp"
```

## Usage Example

```c++
#include "BitVector.hpp"

int main() {
    
    Leszek::BitVector bitVec;

    // Optionally reserve the data to avoid reallocations
    bitVec.reserve(128);

    // Push data of variable bit-widths
    bitVec.pushData(7, 100);       // Push 7 bits
    bitVec.pushData(13, 4095);     // Push 13 bits
    bitVec.pushData(1, 1);         // Push 1 bit (boolean flag)

    // Update existing data in-place
    bitVec.setData(7, 13, 2048); 

    // Access the raw memory buffer for serialization
    const std::vector<uint64_t>& rawBuffer = bitVec.getBuffer();
    size_t totalBitsUsed = bitVec.getSize();

    // Send over network
    uint8_t* data = reinterpret_cast<uint8_t*>(rawBuffer.data());
    size_t dataSize = bitVec.getRequiredBytes(); // (bitSize + 7 / 8)

    connection.sendData(data, dataSize);

    ////////////////
    // Other side //
    ////////////////

    size_t dataSize = 0;
    uint8_t* data = connection.getNextData(&dataSize);

    // Constructor takes in Bit Size instead of Byte Size
    Leszek::BitVector bitVec(data, dataSize * 8);

    // Reconstruct the data
    uint64_t health = bitVec.getData(0, 7);
    uint64_t shield = bitVec.getData(7, 13);
    bool isAlive    = bitVec.getBit(20);

    return 0;
}
```

## Performance Profile

BitVector prioritizes memory density over access speed. Because dynamic bit-packing requires bitwise shifting and boundary checking, it will inherently be slower than accessing a raw `std::array` or `std::vector` of fixed-width types.

**Reference Benchmarks** *(Hardware: i7 9700k 8t 8c, L1 64KB, L2 256KB, L3 12MB, 64GB DDR4 2666 MHz)*

* **Random Memory Access:** ~0.796x (Updating) and ~1.071x (Retrieving) relative to `std::array`.
* **Sequential Memory Access:** ~0.299x (Updating) and ~1.132x (Retrieving) relative to `std::array`.

*Note: Benchmarks were conducted with pre-allocated buffers. The primary bottleneck is `setData(...)` due to the read-modify-write cycle required for bit manipulation. If your application requires high-frequency data mutations over static data retrieval, evaluate whether the memory savings outweigh the CPU cost in your specific pipeline.*

## License

Copyright 2026 @LeszekDev

Licensed under the Apache License, Version 2.0. See the bottom of BitVector.hpp for the full license text.