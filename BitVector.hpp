/*\
 * ======================================================================================
 *  File: BitVector.hpp
 *  Author: @LeszekDev
 *  Version: 1.0.0 (Released at April 11, 2026)
 *  Repository: https://github.com/LeszekDev/BitContainers
 *  Requirements: C++17
 * ======================================================================================
 *  License Agreement can be found at the bottom of this file
 * ======================================================================================
 *  This is a Single-Header library - You copy and paste it into your project.
 *  To set it up you should create a BitVector.cpp (or BitContainers.cpp containing
 *  implementation for every file), do #define LESZEK_BITVECTOR_IMPLEMENTATION and
 *  after the define, you should include the library with #include "BitVector.hpp"
 *  
 *  This is a class similar to std::vector but with a one change - it operates
 *  on bits instead of fixed types and limits user to setting/getting only up
 *  to 64-bit data in the form of uint64_t. That is done for the performance
 *  reasons, as bitpacking with dynamic width data is already pretty expensive.
 *  
 *  Main purpose of this library is to make data store less space, whether it
 *  would be for RAM, disk space or sending data over internet, but it should still
 *  be reserved only for that, as raw std::array or std::vector is still faster
 *  than this implementation.
 * ======================================================================================
 *  Benchmarks on i7 9700k 8t 8c, L1 64KB, L2 256KB, L3 12MB, 64GB DDR4 2666 MHz RAM:
 * 
 *  RandomMemoryAccess:     0.917x (Updating) and 1.073x (Retrieving) Speed of std::array
 *  SequentialMemoryAccess: 0.917x (Updating) and 1.073x (Retrieving) Speed of std::array
 *  (Benchmarks had a preallocated buffers, so BitVector didn't do any reallocations)
 * 
 *  This means that Leszek::BitVector is pretty much always slower than std::array
 *  Especially the Updating (setData(...)) shows that its the main bottleneck.
 *  Thats why you should consider if an std::vector/std::array would be more viable
 *  option for where you do a lot of updating rather than retrieving.
 * ======================================================================================
\*/

///////////////////////
// DECLARATION BELOW //
///////////////////////

#ifndef LESZEK_BITVECTOR_INCLUDE
#define LESZEK_BITVECTOR_INCLUDE

#include <vector>
#include <cstdint>

#ifndef LESZEK_BITVECTOR_RESTRICT
#if defined(_MSC_VER)
#define LESZEK_BITVECTOR_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define LESZEK_BITVECTOR_RESTRICT __restrict__
#else
#define LESZEK_BITVECTOR_RESTRICT
#endif
#endif // LESZEK_BITVECTOR_RESTRICT

namespace Leszek {

	/*\
	 * An optimized vector for storing and retrieving
	 * bit packed data. You can only retrieve/set data
	 * with max size of 64 bits as to save performance.
	\*/
	class BitVector {
	private:

		size_t bitSize = 0;
		std::vector<uint64_t> data;

		// Helper function to safely byteswap a 64-bit integer if on a Big-Endian system.
		// This ensures the in-memory layout remains Little-Endian for serialization.
		static uint64_t byteswap64(uint64_t val);

		static uint64_t loadLE(const uint64_t* LESZEK_BITVECTOR_RESTRICT p);
		static void storeLE(uint64_t* LESZEK_BITVECTOR_RESTRICT p, uint64_t v);

		uint64_t getDataBlock(size_t index) const;
		void setDataBlock(size_t index, uint64_t value);

	public:

		BitVector(size_t size = 0);
		BitVector(uint8_t* data, size_t bitsSize);

		void resize(size_t newBitsSize);
		void reserve(size_t bitsReserved);

		void clear();
		void shrink_to_fit();

		[[nodiscard]] uint64_t getData(size_t bitIndex, int bitWidth) const;
		void setData(size_t bitIndex, int bitWidth, uint64_t value);

		[[nodiscard]] bool getBit(size_t bitIndex) const;
		void setBit(size_t bitIndex, bool value);

		void pushData(int bitWidth, uint64_t value);
		uint64_t popData(int bitWidth);

		// Returns size of buffer IN BITS (not bytes)
		[[nodiscard]] size_t getBitSize() const;

		// Returns minimum amount of bytes required to represent this vector
		[[nodiscard]] size_t getMinimumRequiredBytes() const;

		std::vector<uint64_t>& getBuffer();
		const std::vector<uint64_t>& getBuffer() const;

	};

}

//////////////////////////
// IMPLEMENTATION BELOW //
//////////////////////////

#ifdef LESZEK_BITVECTOR_IMPLEMENTATION

#ifndef LESZEK_BITVECTOR_ASSERT
#include <cassert>
#define LESZEK_BITVECTOR_ASSERT(X, REASON) assert(X && REASON)
#endif // LESZEK_BITVECTOR_ASSERT

#ifndef LESZEK_BITVECTOR_FORCEINLINE
#if defined(_MSC_VER)
#define LESZEK_BITVECTOR_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define LESZEK_BITVECTOR_FORCEINLINE [[gnu::always_inline]] inline
#else
#define LESZEK_BITVECTOR_FORCEINLINE inline
#endif
#endif // LESZEK_BITVECTOR_FORCEINLINE

#ifndef LESZEK_BITVECTOR_IS_LITTLE_ENDIAN
#if __cplusplus >= 202002L
#include <bit>
#define LESZEK_BITVECTOR_IS_LITTLE_ENDIAN (std::endian::native == std::endian::little)
#else // __cplusplus >= 202002L ( C++ 20)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LESZEK_BITVECTOR_IS_LITTLE_ENDIAN 0
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LESZEK_BITVECTOR_IS_LITTLE_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) || defined(_M_IX86) || defined(_M_X64)
	// _M_IX86 and _M_X64 are specific to MSVC (Windows)
#define LESZEK_BITVECTOR_IS_LITTLE_ENDIAN 1
#else
#error "Unknown architecture endianness"
#endif
#endif // __cplusplus >= 202002L ( C++ 20)
#endif //LESZEK_BITVECTOR_ENDIANESS

// This is for making byteswap64() an "alias" for std::byteswap(val)
#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
#include <bit>
#endif

namespace Leszek {

	LESZEK_BITVECTOR_FORCEINLINE uint64_t BitVector::byteswap64(uint64_t val) {

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
		return std::byteswap(val);
#elif defined(_MSC_VER)
		return _byteswap_uint64(val);
#elif defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap64(val);
#else
		return
			((val & 0xFF00000000000000ULL) >> 56) |
			((val & 0x00FF000000000000ULL) >> 40) |
			((val & 0x0000FF0000000000ULL) >> 24) |
			((val & 0x000000FF00000000ULL) >> 8) |
			((val & 0x00000000FF000000ULL) << 8) |
			((val & 0x0000000000FF0000ULL) << 24) |
			((val & 0x000000000000FF00ULL) << 40) |
			((val & 0x00000000000000FFULL) << 56);
#endif

	}

	LESZEK_BITVECTOR_FORCEINLINE uint64_t BitVector::loadLE(const uint64_t* LESZEK_BITVECTOR_RESTRICT p) {
		if constexpr (LESZEK_BITVECTOR_IS_LITTLE_ENDIAN)
			return *p;
		else
			return byteswap64(*p);
	}
	LESZEK_BITVECTOR_FORCEINLINE void BitVector::storeLE(uint64_t* LESZEK_BITVECTOR_RESTRICT p, uint64_t v) {
		if constexpr (LESZEK_BITVECTOR_IS_LITTLE_ENDIAN)
			*p = v;
		else
			*p = byteswap64(v);
	}

	LESZEK_BITVECTOR_FORCEINLINE uint64_t BitVector::getDataBlock(size_t index) const {
		if constexpr (LESZEK_BITVECTOR_IS_LITTLE_ENDIAN)
			return *(data.data() + index);
		else
			return byteswap64(*(data.data() + index));
	}
	LESZEK_BITVECTOR_FORCEINLINE void BitVector::setDataBlock(size_t index, uint64_t value) {
		if constexpr (LESZEK_BITVECTOR_IS_LITTLE_ENDIAN)
			*(data.data() + index) = value;
		else
			*(data.data() + index) = byteswap64(value);
	}

	void BitVector::resize(size_t newBitsSize) {
		if (newBitsSize == bitSize) return;
		bitSize = newBitsSize;

		const size_t requiredBlocks = (newBitsSize + 63) >> 6;
		if (requiredBlocks != data.size()) {
			data.resize(requiredBlocks, 0);
		}
	}
	void BitVector::reserve(size_t bitsReserved) {
		if (bitsReserved == 0) return;
		size_t requiredBlocks = ((bitsReserved + 63) / 64);
		data.reserve(requiredBlocks);
	}

	void BitVector::clear() {
		data.clear();
		bitSize = 0;
	}
	void BitVector::shrink_to_fit() {
		data.shrink_to_fit();
	}

	BitVector::BitVector(size_t size) {
		resize(size);
	}

	BitVector::BitVector(uint8_t* data, size_t bitsSize) {
		resize(bitsSize);
		memcpy(this->data.data(), data, (bitsSize + 7) / 8);
	}

	[[nodiscard]] uint64_t BitVector::getData(size_t bitIndex, int bitWidth) const {
		if (bitWidth == 0) return 0;

		LESZEK_BITVECTOR_ASSERT(bitWidth >= 0 && bitWidth <= 64, "bitWidth outside of range <0,64>");
		LESZEK_BITVECTOR_ASSERT(bitIndex >= 0 && bitIndex + bitWidth <= bitSize, "index out of bounds!");

		const size_t block = bitIndex >> 6;
		const size_t offset = bitIndex & 63;
		const uint64_t bitmask = ~0ULL >> (64 - bitWidth);
		const uint64_t* LESZEK_BITVECTOR_RESTRICT ptr = data.data() + block;

		// Grab the first block
		uint64_t value = loadLE(ptr) >> offset;

		// The perfectly predictable boundary stitch
		if (offset + bitWidth > 64) {
			value |= loadLE(ptr + 1) << (64 - offset);
		}

		return static_cast<uint64_t>(value & bitmask);
	}
	void BitVector::setData(size_t bitIndex, int bitWidth, uint64_t value) {
		if (bitWidth == 0) return;

		LESZEK_BITVECTOR_ASSERT(bitWidth >= 0 && bitWidth <= 64, "bitWidth outside of range <0,64>");
		LESZEK_BITVECTOR_ASSERT(bitIndex >= 0 && bitIndex + bitWidth <= bitSize, "index out of bounds!");

		const size_t block = bitIndex >> 6;
		const size_t offset = bitIndex & 63;
		uint64_t* LESZEK_BITVECTOR_RESTRICT ptr = data.data() + block;

		if (offset == 0 && bitWidth == 64) {
			storeLE(ptr, value);
			return;
		}

		const uint64_t bitmask = ~0ULL >> (64 - bitWidth);
		const uint64_t cleanValue = value & bitmask;

		// Update first block
		storeLE(ptr, (loadLE(ptr) & ~(bitmask << offset)) | (cleanValue << offset));

		// If we crossed the boundary, update the second block
		if (offset + bitWidth > 64) [[unlikely]] {
			const int bitsLeft = 64 - offset;
			storeLE(ptr + 1, (loadLE(ptr + 1) & ~(bitmask >> bitsLeft)) | (cleanValue >> bitsLeft));
		}
	}

	[[nodiscard]] bool BitVector::getBit(size_t bitIndex) const {
		LESZEK_BITVECTOR_ASSERT(bitIndex >= 0 && bitIndex < bitSize, "index out of bounds!");
		const uint64_t* LESZEK_BITVECTOR_RESTRICT ptr = data.data() + (bitIndex >> 6);
		return static_cast<bool>((loadLE(ptr) >> (bitIndex & 63)) & 1ULL);
	}
	void BitVector::setBit(size_t bitIndex, bool value) {
		LESZEK_BITVECTOR_ASSERT(bitIndex >= 0 && bitIndex < bitSize, "index out of bounds!");
		const int offset = static_cast<int>(bitIndex & 63);
		uint64_t* LESZEK_BITVECTOR_RESTRICT ptr = data.data() + (bitIndex >> 6);
		storeLE(ptr, (loadLE(ptr) & ~(1ULL << offset)) | (static_cast<uint64_t>(value) << offset));
	}

	void BitVector::pushData(int bitWidth, uint64_t value) {
		if (bitWidth == 0) return;

		LESZEK_BITVECTOR_ASSERT(bitWidth >= 0 && bitWidth <= 64, "bitWidth outside of range <0,64>");

		const size_t writeAt = bitSize;
		bitSize += static_cast<size_t>(bitWidth);

		if (((bitSize + 63) >> 6) > data.size()) [[unlikely]] {
			// At most one new block needed (bitWidth <= 64).
			data.emplace_back(0);
		}

		setData(writeAt, bitWidth, value);
	}
	uint64_t BitVector::popData(int bitWidth) {
		if (bitWidth == 0) return 0;

		LESZEK_BITVECTOR_ASSERT(bitWidth > 0 && bitWidth <= 64, "bitWidth outside of range <0,64>");
		LESZEK_BITVECTOR_ASSERT(bitSize >= bitWidth, "not enough bits to pop!");

		const uint64_t value = getData(bitSize - bitWidth, bitWidth);
		bitSize -= static_cast<size_t>(bitWidth);

		const size_t neededBlocks = (bitSize + 63) >> 6;
		if (neededBlocks < data.size()) [[unlikely]] {
			data.resize(neededBlocks);
		}

		return value;
	}

	[[nodiscard]] size_t BitVector::getBitSize() const {
		return bitSize;
	}

	[[nodiscard]] size_t BitVector::getMinimumRequiredBytes() const {
		return (bitSize + 7) / 8;
	}

	std::vector<uint64_t>& BitVector::getBuffer() {
		return data;
	}
	const std::vector<uint64_t>& BitVector::getBuffer() const {
		return data;
	}

}

#endif // LESZEK_BITVECTOR_IMPLEMENTATION

#endif // LESZEK_BITVECTOR_INCLUDE

/*\
 * ======================================================================================
 *  License:
 * ======================================================================================
 *  Copyright 2026 @LeszekDev
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 * ======================================================================================
\*/