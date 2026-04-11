#include <array>
#include <memory>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <bit>

#include <sstream>

#include "../../BitVector.hpp"
#include "../FastRandom.hpp"

namespace MinecraftChunk {

	template<typename T, int CHUNK_SIZE>
	class ArrayChunk {
	public:

		std::array<T, CHUNK_SIZE> data;

		inline void setBlock(size_t index, T block) {
			data[index] = block;
		}
		T getBlock(size_t index) {
			return data[index];
		}

	};

	template<int BITS_PER_BLOCK, int CHUNK_SIZE>
	class BitVectorChunk {
	public:

		Leszek::BitVector data;
		BitVectorChunk() {
			data.resize(CHUNK_SIZE * BITS_PER_BLOCK);
		}

		inline void setBlock(size_t index, uint64_t block) {
			data.setData(index * BITS_PER_BLOCK, BITS_PER_BLOCK, block);
		}
		uint64_t getBlock(size_t index) {
			return data.getData(index * BITS_PER_BLOCK, BITS_PER_BLOCK);
		}

	};

	template<typename ARRAY_BLOCK_TYPE, int BITS_PER_BLOCK, int SIZE>
	void runRandomBlockBenchmark(uint64_t randomGeneratorSeed, std::vector<double>& average_setBlock, std::vector<double>& average_getBlock) {

		int sizeSqrt3 = std::pow(SIZE, 1 / 3.0f);

		std::cout << "Running RandomBlockBenchmark with " 
			<< (sizeof(ARRAY_BLOCK_TYPE) * 8) << "-Bit std::array vs " 
			<< BITS_PER_BLOCK << "-Bit Leszek::BitVector (Size: " 
			<< SIZE << " blocks, or " << sizeSqrt3 << "x" << sizeSqrt3 << "x" << sizeSqrt3 << ")" << std::endl;

		std::cout << std::setprecision(3) << std::fixed;

		// Unique pointer so they won't be on stack and won't be influenced by
		// L1 cache from this running program before benchmark started
		std::unique_ptr<ArrayChunk<ARRAY_BLOCK_TYPE, SIZE>> 
			arrayChunk = std::make_unique<ArrayChunk<ARRAY_BLOCK_TYPE, SIZE>>();
		std::unique_ptr<BitVectorChunk<BITS_PER_BLOCK, SIZE>> 
			bitVectorChunk = std::make_unique<BitVectorChunk<BITS_PER_BLOCK, SIZE>>();

		// Try to maybe reset CPU cache
		auto resetCache = []() {
			volatile size_t sum = 0;
			for (int i = 0; i < 1024 * 128; i++) {
				std::unique_ptr<std::array<uint8_t, 512>> someDataInRandomMemoryPlace = std::make_unique<std::array<uint8_t, 512>>();
				sum += reinterpret_cast<size_t>(someDataInRandomMemoryPlace.get());
				sum += (*someDataInRandomMemoryPlace)[i & 511];
			}
		};

		constexpr int RANDOM_AMOUNTS[] = {
			1 << 8, 
			1 << 12, 
			1 << 16, 
			1 << 18,
			1 << 20,
			1 << 22,
			1 << 24,
		};

		////////////////
		// setBlock() //
		////////////////
		for (int i = 0; i < sizeof(RANDOM_AMOUNTS) / sizeof(int); i++) {
			const int amount = RANDOM_AMOUNTS[i];

			double elapsedArray = 0;
			double elapsedBitVector = 0;

			std::cout << "setBlock() x " << amount << ":\t";

			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				for (uint32_t j = 0; j < amount; ++j) {
					uint32_t index = random.next() % SIZE;
					uint64_t block = (random.next() << 32) | random.next();
					arrayChunk->setBlock(index, (ARRAY_BLOCK_TYPE)block);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedArray = elapsed.count();

				std::cout << " std::array = " << elapsedArray << " ms VS " << std::flush;

			}
			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				for (uint32_t j = 0; j < amount; ++j) {
					uint32_t index = random.next() % SIZE;
					uint64_t block = (random.next() << 32) | random.next();
					bitVectorChunk->setBlock(index, block);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedBitVector = elapsed.count();

				std::cout << " Leszek::BitVector = " << elapsedBitVector << " ms" << std::flush;
			}

			double timesFaster = elapsedArray / elapsedBitVector;
			average_setBlock.push_back(timesFaster);

			std::cout << " (" << timesFaster << "x Faster)" << std::endl;
		}

		////////////////
		// getBlock() //
		////////////////
		for (int i = 0; i < sizeof(RANDOM_AMOUNTS) / sizeof(int); i++) {
			const int amount = RANDOM_AMOUNTS[i];

			double elapsedArray = 0;
			double elapsedBitVector = 0;

			std::cout << "getBlock() x " << amount << ":\t";

			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				volatile uint64_t sum = 0;

				for (uint32_t j = 0; j < amount; ++j) {
					uint32_t index = random.next() % SIZE;
					sum += arrayChunk->getBlock(index);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedArray = elapsed.count();

				std::cout << " std::array = " << elapsedArray << " ms VS " << std::flush;

			}
			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				volatile uint64_t sum = 0;

				for (uint32_t j = 0; j < amount; ++j) {
					uint32_t index = random.next() % SIZE;
					sum += bitVectorChunk->getBlock(index);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedBitVector = elapsed.count();

				std::cout << " Leszek::BitVector = " << elapsedBitVector << " ms" << std::flush;
			}

			double timesFaster = elapsedArray / elapsedBitVector;
			average_getBlock.push_back(timesFaster);

			std::cout << " (" << timesFaster << "x Faster)" << std::endl;
		}

		std::cout << std::endl;
	}

	template<typename ARRAY_BLOCK_TYPE, int BITS_PER_BLOCK, int SIZE>
	void runSequentialBlockBenchmark(uint64_t randomGeneratorSeed, std::vector<double>& average_setBlock, std::vector<double>& average_getBlock) {

		int sizeSqrt3 = std::pow(SIZE, 1 / 3.0f);

		std::cout << "Running SequentialBlockBenchmark with "
			<< (sizeof(ARRAY_BLOCK_TYPE) * 8) << "-Bit std::array vs "
			<< BITS_PER_BLOCK << "-Bit Leszek::BitVector (Size: "
			<< SIZE << " blocks, or " << sizeSqrt3 << "x" << sizeSqrt3 << "x" << sizeSqrt3 << ")" << std::endl;

		std::cout << std::setprecision(3) << std::fixed;

		// Unique pointer so they won't be on stack and won't be influenced by
		// L1 cache from this running program before benchmark started
		std::unique_ptr<ArrayChunk<ARRAY_BLOCK_TYPE, SIZE>>
			arrayChunk = std::make_unique<ArrayChunk<ARRAY_BLOCK_TYPE, SIZE>>();
		std::unique_ptr<BitVectorChunk<BITS_PER_BLOCK, SIZE>>
			bitVectorChunk = std::make_unique<BitVectorChunk<BITS_PER_BLOCK, SIZE>>();

		// Try to maybe reset CPU cache
		auto resetCache = []() {
			volatile size_t sum = 0;
			for (int i = 0; i < 1024 * 128; i++) {
				std::unique_ptr<std::array<uint8_t, 512>> someDataInRandomMemoryPlace = std::make_unique<std::array<uint8_t, 512>>();
				sum += reinterpret_cast<size_t>(someDataInRandomMemoryPlace.get());
				sum += (*someDataInRandomMemoryPlace)[i & 511];
			}
			};

		constexpr int RANDOM_AMOUNTS[] = {
			1 << 8,
			1 << 12,
			1 << 16,
			1 << 18,
			1 << 20,
			1 << 22,
			1 << 24,
		};

		////////////////
		// setBlock() //
		////////////////
		for (int i = 0; i < sizeof(RANDOM_AMOUNTS) / sizeof(int); i++) {
			const int amount = RANDOM_AMOUNTS[i];

			double elapsedArray = 0;
			double elapsedBitVector = 0;

			std::cout << "setBlock() x " << amount << ":\t";

			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				for (uint32_t j = 0; j < amount; ++j) {
					arrayChunk->setBlock(j % SIZE, (ARRAY_BLOCK_TYPE)j);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedArray = elapsed.count();

				std::cout << " std::array = " << elapsedArray << " ms VS " << std::flush;

			}
			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				for (uint32_t j = 0; j < amount; ++j) {
					bitVectorChunk->setBlock(j % SIZE, j);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedBitVector = elapsed.count();

				std::cout << " Leszek::BitVector = " << elapsedBitVector << " ms" << std::flush;
			}

			double timesFaster = elapsedArray / elapsedBitVector;
			average_setBlock.push_back(timesFaster);

			std::cout << " (" << timesFaster << "x Faster)" << std::endl;
		}

		////////////////
		// getBlock() //
		////////////////
		for (int i = 0; i < sizeof(RANDOM_AMOUNTS) / sizeof(int); i++) {
			const int amount = RANDOM_AMOUNTS[i];

			double elapsedArray = 0;
			double elapsedBitVector = 0;

			std::cout << "getBlock() x " << amount << ":\t";

			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				volatile uint64_t sum = 0;

				for (uint32_t j = 0; j < amount; ++j) {
					sum += arrayChunk->getBlock(j % SIZE);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedArray = elapsed.count();

				std::cout << " std::array = " << elapsedArray << " ms VS " << std::flush;

			}
			{
				FastRandom random(randomGeneratorSeed);

				// Reset cache
				resetCache();
				auto start = std::chrono::high_resolution_clock::now();

				volatile uint64_t sum = 0;

				for (uint32_t j = 0; j < amount; ++j) {
					sum += bitVectorChunk->getBlock(j % SIZE);
				}

				auto end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double, std::milli> elapsed = end - start;
				elapsedBitVector = elapsed.count();

				std::cout << " Leszek::BitVector = " << elapsedBitVector << " ms" << std::flush;
			}

			double timesFaster = elapsedArray / elapsedBitVector;
			average_getBlock.push_back(timesFaster);

			std::cout << " (" << timesFaster << "x Faster)" << std::endl;
		}

		std::cout << std::endl;
	}

	// This struct contains data about how much faster is Leszek::BitVector compared to std::array in retrieving and modifying the data
	struct BenchmarkData {
		struct {
			struct { double avg_setBlock = 0, avg_getBlock = 0; } common, all;
			int tests = 0;
		} RandomBlockBenchmark, SequentialBlockBenchmark;

		std::string toString() {
			std::stringstream ss;

			ss << std::setprecision(3) << std::fixed;

			ss << "runRandomBlockBenchmark (" << RandomBlockBenchmark.tests << " configurations): [setBlock, getBlock]" << std::endl;
			ss << "On average Leszek::BitVector is [" << RandomBlockBenchmark.common.avg_setBlock << ","
				<< RandomBlockBenchmark.common.avg_getBlock << "] times faster than std::array in common chunk cases!" << std::endl;
			ss << "On average Leszek::BitVector is [" << RandomBlockBenchmark.all.avg_setBlock << ","
				<< RandomBlockBenchmark.all.avg_getBlock << "] times faster than std::array in all tests!" << std::endl;
			ss << std::endl;

			ss << "runSequentialBlockBenchmark (" << SequentialBlockBenchmark.tests << " configurations): [setBlock, getBlock]" << std::endl;
			ss << "On average Leszek::BitVector is [" << SequentialBlockBenchmark.common.avg_setBlock << ","
				<< SequentialBlockBenchmark.common.avg_getBlock << "] times faster than std::array in common chunk cases!" << std::endl;
			ss << "On average Leszek::BitVector is [" << SequentialBlockBenchmark.all.avg_setBlock << ","
				<< SequentialBlockBenchmark.all.avg_getBlock << "] times faster than std::array in all tests!" << std::endl;
			ss << std::endl;

			return ss.str();
		}
	};

	template<typename T, int BITS_PER_BLOCK>
	void runRandomBlockBenchmarkGroup(std::vector<double>& average_setBlock, std::vector<double>& average_getBlock) {
		MinecraftChunk::runRandomBlockBenchmark<T, BITS_PER_BLOCK, 8 * 8 * 8>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runRandomBlockBenchmark<T, BITS_PER_BLOCK, 16 * 16 * 16>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runRandomBlockBenchmark<T, BITS_PER_BLOCK, 32 * 32 * 32>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runRandomBlockBenchmark<T, BITS_PER_BLOCK, 64 * 64 * 64>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runRandomBlockBenchmark<T, BITS_PER_BLOCK, 128 * 128 * 128>(12345, average_setBlock, average_getBlock);
	}

	template<typename T, int BITS_PER_BLOCK>
	void runSequentialBlockBenchmarkGroup(std::vector<double>& average_setBlock, std::vector<double>& average_getBlock) {
		MinecraftChunk::runSequentialBlockBenchmark<T, BITS_PER_BLOCK, 8 * 8 * 8>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runSequentialBlockBenchmark<T, BITS_PER_BLOCK, 16 * 16 * 16>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runSequentialBlockBenchmark<T, BITS_PER_BLOCK, 32 * 32 * 32>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runSequentialBlockBenchmark<T, BITS_PER_BLOCK, 64 * 64 * 64>(12345, average_setBlock, average_getBlock);
		MinecraftChunk::runSequentialBlockBenchmark<T, BITS_PER_BLOCK, 128 * 128 * 128>(12345, average_setBlock, average_getBlock);
	}

	BenchmarkData runBenchmark() {
		BenchmarkData data;


		///////////////////////////////////////////////
		// MinecraftChunk::runRandomBlockBenchmark() //
		///////////////////////////////////////////////
		{
			// How many times faster is Leszek::BitVector than std::array
			std::vector<double> average_setBlock, average_getBlock;

			runRandomBlockBenchmarkGroup<uint8_t, 4>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint8_t, 6>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint8_t, 8>(average_setBlock, average_getBlock);

			runRandomBlockBenchmarkGroup<uint32_t, 6>(average_setBlock, average_getBlock);	// 64 blocks (for example underground, pretty rare) vs raw uint32_t blockType
			runRandomBlockBenchmarkGroup<uint32_t, 8>(average_setBlock, average_getBlock);	// 256 blocks (probably the most common) vs raw uint32_t blockType
			runRandomBlockBenchmarkGroup<uint32_t, 10>(average_setBlock, average_getBlock);	// 1024 blocks per chunk (block palette) vs raw uint32_t blockType

			runRandomBlockBenchmarkGroup<uint64_t, 6>(average_setBlock, average_getBlock);	// 64 blocks (for example underground, pretty rare) vs raw uint64_t blockType
			runRandomBlockBenchmarkGroup<uint64_t, 8>(average_setBlock, average_getBlock);	// 256 blocks (probably the most common) vs raw uint64_t blockType
			runRandomBlockBenchmarkGroup<uint64_t, 10>(average_setBlock, average_getBlock);	// 1024 blocks per chunk (block palette) vs raw uint64_t blockType

			for (double d : average_setBlock) data.RandomBlockBenchmark.common.avg_setBlock += d;
			data.RandomBlockBenchmark.common.avg_setBlock /= average_setBlock.size();

			for (double d : average_getBlock) data.RandomBlockBenchmark.common.avg_getBlock += d;
			data.RandomBlockBenchmark.common.avg_getBlock /= average_getBlock.size();

			runRandomBlockBenchmarkGroup<uint64_t, 12>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint64_t, 14>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint64_t, 16>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint64_t, 20>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint64_t, 24>(average_setBlock, average_getBlock);

			runRandomBlockBenchmarkGroup<uint16_t, 16>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint32_t, 32>(average_setBlock, average_getBlock);
			runRandomBlockBenchmarkGroup<uint64_t, 64>(average_setBlock, average_getBlock);

			for (double d : average_setBlock) data.RandomBlockBenchmark.all.avg_setBlock += d;
			data.RandomBlockBenchmark.all.avg_setBlock /= average_setBlock.size();

			for (double d : average_getBlock) data.RandomBlockBenchmark.all.avg_getBlock += d;
			data.RandomBlockBenchmark.all.avg_getBlock /= average_getBlock.size();

			data.RandomBlockBenchmark.tests = average_setBlock.size();
		}

		///////////////////////////////////////////////////
		// MinecraftChunk::runSequentialBlockBenchmark() //
		///////////////////////////////////////////////////
		{
			// How many times faster is Leszek::BitVector than std::array
			std::vector<double> average_setBlock, average_getBlock;

			runSequentialBlockBenchmarkGroup<uint8_t, 4>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint8_t, 6>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint8_t, 8>(average_setBlock, average_getBlock);

			runSequentialBlockBenchmarkGroup<uint32_t, 6>(average_setBlock, average_getBlock);	// 64 blocks (for example underground, pretty rare) vs raw uint32_t blockType
			runSequentialBlockBenchmarkGroup<uint32_t, 8>(average_setBlock, average_getBlock);	// 256 blocks (probably the most common) vs raw uint32_t blockType
			runSequentialBlockBenchmarkGroup<uint32_t, 10>(average_setBlock, average_getBlock);	// 1024 blocks per chunk (block palette) vs raw uint32_t blockType

			runSequentialBlockBenchmarkGroup<uint64_t, 6>(average_setBlock, average_getBlock);	// 64 blocks (for example underground, pretty rare) vs raw uint64_t blockType
			runSequentialBlockBenchmarkGroup<uint64_t, 8>(average_setBlock, average_getBlock);	// 256 blocks (probably the most common) vs raw uint64_t blockType
			runSequentialBlockBenchmarkGroup<uint64_t, 10>(average_setBlock, average_getBlock);	// 1024 blocks per chunk (block palette) vs raw uint64_t blockType

			for (double d : average_setBlock) data.SequentialBlockBenchmark.common.avg_setBlock += d;
			data.SequentialBlockBenchmark.common.avg_setBlock /= average_setBlock.size();

			for (double d : average_getBlock) data.SequentialBlockBenchmark.common.avg_getBlock += d;
			data.SequentialBlockBenchmark.common.avg_getBlock /= average_getBlock.size();

			runSequentialBlockBenchmarkGroup<uint64_t, 12>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint64_t, 14>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint64_t, 16>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint64_t, 20>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint64_t, 24>(average_setBlock, average_getBlock);

			runSequentialBlockBenchmarkGroup<uint16_t, 16>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint32_t, 32>(average_setBlock, average_getBlock);
			runSequentialBlockBenchmarkGroup<uint64_t, 64>(average_setBlock, average_getBlock);

			for (double d : average_setBlock) data.SequentialBlockBenchmark.all.avg_setBlock += d;
			data.SequentialBlockBenchmark.all.avg_setBlock /= average_setBlock.size();

			for (double d : average_getBlock) data.SequentialBlockBenchmark.all.avg_getBlock += d;
			data.SequentialBlockBenchmark.all.avg_getBlock /= average_getBlock.size();

			data.SequentialBlockBenchmark.tests = average_setBlock.size();
		}

		return data;
	}

}