#include <exception>
#include <stdexcept>

#define LESZEK_BITVECTOR_IMPLEMENTATION
#ifndef LESZEK_BITVECTOR_ASSERT
#define LESZEK_BITVECTOR_ASSERT(X, REASON) if(!(X)) { throw std::runtime_error(REASON); }
#endif
#include "../BitVector.hpp"

#include "SimpleUnitTestFramework.hpp"

UnitTestCollection getFullTestCollection() {
	UnitTestCollection tests({
		UnitTest("Initialization", []() { Leszek::BitVector bv; }),
		UnitTest("Empty Vector getBitSize() == 0", []() { Leszek::BitVector bv; return bv.size() == 0; }),
		UnitTest("Initialization with Size", []() { Leszek::BitVector bv(162); return bv.size() == 162; }),
		UnitTest("resize() Test", []() { Leszek::BitVector bv; bv.resize(100); }),
		UnitTest("getBitSize() Test", []() { Leszek::BitVector bv; bv.resize(100); return bv.size() == 100; }),
		UnitTest("Underlying Vector Size Test", []() {
			Leszek::BitVector bv; bv.resize(100);
			// 100 bits would require at least 2 uint64_t's
			return bv.get_buffer().size() >= 2;
		}),
		UnitTest("clear() Test", []() {
			Leszek::BitVector bv; bv.resize(100);
			bv.clear();
			return bv.size() == 0 && bv.get_buffer().size() == 0;
		}),
		UnitTest("Set and Get Test (Single 64-Bit Block)", []() {
			Leszek::BitVector bv(64);
			bv.set(0, 8, 0xFF);         // Set first 8 bits
			bv.set(8, 16, 0xABCD);      // Set next 16 bits
			bv.set(24, 4, 0x5);         // Set next 4 bits

			if (bv.at(0, 8) != 0xFF) return false;
			if (bv.at(8, 16) != 0xABCD) return false;
			if (bv.at(24, 4) != 0x5) return false;

			// Ensure the remaining bits are still 0
			if (bv.at(28, 36) != 0x0) return false;
			return true;
		}),
		UnitTest("64-Bit Boundary Crossing", []() {
			Leszek::BitVector bv(128);

			// Write a 32-bit value starting at bit 50.
			// This physically spans across block 0 (bits 0-63) and block 1 (bits 64-127).
			uint64_t testVal = 0x12345678;
			bv.set(50, 32, testVal);
			if (bv.at(50, 32) != testVal) return false;

			// Cross boundary with exactly 64 bits of data
			uint64_t bigVal = 0xDEADBEEFCAFEBABE;
			bv.set(32, 64, bigVal);
			if (bv.at(32, 64) != bigVal) return false;
			return true;
		}),
		UnitTest("Maximum Bit Width (64 bits)", []() {
			Leszek::BitVector bv(128);
			uint64_t maxVal1 = 0xFFFFFFFFFFFFFFFF; // All 1s
			uint64_t maxVal2 = 0x0123456789ABCDEF; // Random large number

			// Perfectly aligned 64-bit set/get
			bv.set(0, 64, maxVal1);
			if (bv.at(0, 64) != maxVal1) return false;

			// Unaligned 64-bit set/get
			bv.set(13, 64, maxVal2);
			if (bv.at(13, 64) != maxVal2) return false;
			return true;
		}),
		UnitTest("Push and Pop Data Dynamically", []() {
			Leszek::BitVector bv;

			bv.push_back(10, 0x3FF); // Push 10 bits
			if (bv.size() != 10) return false;

			bv.push_back(60, 0x123456789ABCDEF); // Push 60 bits (causes boundary cross)
			if (bv.size() != 70) return false;

			bv.push_back(3, 0x5); // Push 3 bits
			if (bv.size() != 73) return false;

			// Pop in reverse order to verify LIFO consistency
			if (bv.pop_back(3) != 0x5) return false;
			if (bv.size() != 70) return false;

			// We only pushed 60 bits, so we need to mask the literal to compare accurately
			uint64_t mask60 = (1ULL << 60) - 1;
			if (bv.pop_back(60) != (0x123456789ABCDEF & mask60)) return false;
			if (bv.size() != 10) return false;

			if (bv.pop_back(10) != 0x3FF) return false;
			if (bv.size() != 0) return false;

			return true;
		}),
		UnitTest("Zero Width Test #1", []() {
			Leszek::BitVector bv;
			bv.set(10, 0, 0xFF);
			return bv.at(10, 0) == 0;
		}),
		UnitTest("Zero Width Test #2", []() {
			Leszek::BitVector bv;
			bv.set(10, 0, 0xFF);
			return bv.size() == 0;
		}),
		UnitTest("Zero Width Test #3", []() {
			Leszek::BitVector bv(127);
			bv.set(10, 0, 0xFF);
			return bv.size() == 127;
		}),
		UnitTest("Zero Width Test #4", []() {
			Leszek::BitVector bv;
			bv.push_back(0, 0xFF);
			return bv.at(10, 0) == 0;
		}),
		UnitTest("Zero Width Test #5", []() {
			Leszek::BitVector bv;
			bv.push_back(0, 0xFF);
			return bv.size() == 0;
		}),
		UnitTest("Zero Width Test #6", []() {
			Leszek::BitVector bv(127);
			bv.push_back(0, 0xFF);
			return bv.size() == 127;
		}),
		UnitTest("Zero Width Test #7", []() {
			Leszek::BitVector bv(127);
			return bv.pop_back(0) == 0;
		}),
		UnitTest("Zero Width Test #8", []() {
			Leszek::BitVector bv(127);
			bv.pop_back(0);
			return bv.size() == 127;
		}),
		UnitTest("Value Masking Test", []() {
			Leszek::BitVector bv(8);
			bv.set(0, 4, 0xFF);
			return bv.at(0, 4) == 0xF;
		}),
		UnitTest("Adjacent Data Bleed (Ensuring neighboring bits are safe)", []() {
			Leszek::BitVector bv(64);
			bv.set(0, 64, 0); // Init all to 0

			// Set alternating 10-bit chunks to see if writes overwrite each other
			bv.set(10, 10, 0x3FF); // Chunk 1: All 1s
			bv.set(20, 10, 0x000); // Chunk 2: All 0s
			bv.set(30, 10, 0x3FF); // Chunk 3: All 1s

			if (bv.at(0, 10) != 0) return false;
			if (bv.at(10, 10) != 0x3FF) return false;
			if (bv.at(20, 10) != 0) return false;
			if (bv.at(30, 10) != 0x3FF) return false;
			if (bv.at(40, 24) != 0) return false; // Everything past bit 40 should still be 0

			// Try another order
			bv.set(30, 10, 0x3FF); // Chunk 3: All 1s
			bv.set(20, 10, 0x000); // Chunk 2: All 0s
			bv.set(10, 10, 0x3FF); // Chunk 1: All 1s

			if (bv.at(0, 10) != 0) return false;
			if (bv.at(10, 10) != 0x3FF) return false;
			if (bv.at(20, 10) != 0) return false;
			if (bv.at(30, 10) != 0x3FF) return false;
			if (bv.at(40, 24) != 0) return false; // Everything past bit 40 should still be 0

			return true;
		}),
	});

	return tests;
}