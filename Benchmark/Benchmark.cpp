#define LESZEK_BITVECTOR_ASSERT(X, REASON)

#include "../Test/TestCollection.hpp"
#undef LESZEK_BITVECTOR_IMPLEMENTATION

#include "Tests/MinecraftChunk.hpp";

#include <thread>

int main() {

	// Run tests at start. If after trying to optimize 
	// something there is an error, no point in testing.
	getFullTestCollection().runTests();

	// Below is Benchmarking Code.
	// Info: For simple benchmark just do:
	// std::cout << MinecraftChunk::runBenchmark().toString();
	// Benchmark below is supposed to get the best average possible
	// and so can take a long time to finish
	
	constexpr int TEST_AMOUNTS[] = {
		32, 32, 32, 32, 32,
		16, 16, 16, 16, 16,
		8,  8,  8,  8,  8,
		4,  4,  4,  4,  4,
		2,  2,  2,  2,  2,
		1,  1,  1,  1,  1,
	};

	//constexpr int TEST_AMOUNTS[] = { 1 };

	MinecraftChunk::BenchmarkData totalAverage;


	for (int testSession = 0; testSession < sizeof(TEST_AMOUNTS) / sizeof(int); testSession++) {
		system((std::string("title " + std::to_string(testSession + 1) + " / " + std::to_string(sizeof(TEST_AMOUNTS) / sizeof(int)))).c_str());

		int TEST_AMOUNT = TEST_AMOUNTS[testSession];

		std::vector<std::thread> threads;

		MinecraftChunk::BenchmarkData data[TEST_AMOUNTS[0]];
		for (int i = 0; i < TEST_AMOUNT; i++) {
			threads.push_back(std::move(std::thread([&](int index) {
				data[index] = MinecraftChunk::runBenchmark();
				}, i)));
		}

		for (int i = 0; i < TEST_AMOUNT; i++) {
			threads[i].join();
		}

		MinecraftChunk::BenchmarkData averageData;
		for (int i = 0; i < TEST_AMOUNT; i++) {
			averageData.RandomBlockBenchmark.all.avg_getBlock += data[i].RandomBlockBenchmark.all.avg_getBlock / (double)TEST_AMOUNT;
			averageData.RandomBlockBenchmark.all.avg_setBlock += data[i].RandomBlockBenchmark.all.avg_setBlock / (double)TEST_AMOUNT;
			averageData.RandomBlockBenchmark.common.avg_getBlock += data[i].RandomBlockBenchmark.common.avg_getBlock / (double)TEST_AMOUNT;
			averageData.RandomBlockBenchmark.common.avg_setBlock += data[i].RandomBlockBenchmark.common.avg_setBlock / (double)TEST_AMOUNT;

			averageData.SequentialBlockBenchmark.all.avg_getBlock += data[i].SequentialBlockBenchmark.all.avg_getBlock / (double)TEST_AMOUNT;
			averageData.SequentialBlockBenchmark.all.avg_setBlock += data[i].SequentialBlockBenchmark.all.avg_setBlock / (double)TEST_AMOUNT;
			averageData.SequentialBlockBenchmark.common.avg_getBlock += data[i].SequentialBlockBenchmark.common.avg_getBlock / (double)TEST_AMOUNT;
			averageData.SequentialBlockBenchmark.common.avg_setBlock += data[i].SequentialBlockBenchmark.common.avg_setBlock / (double)TEST_AMOUNT;

			averageData.RandomBlockBenchmark.tests = data[i].RandomBlockBenchmark.tests;
			averageData.SequentialBlockBenchmark.tests = data[i].SequentialBlockBenchmark.tests;
		}


		totalAverage.RandomBlockBenchmark.all.avg_getBlock += averageData.RandomBlockBenchmark.all.avg_getBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.RandomBlockBenchmark.all.avg_setBlock += averageData.RandomBlockBenchmark.all.avg_setBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.RandomBlockBenchmark.common.avg_getBlock += averageData.RandomBlockBenchmark.common.avg_getBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.RandomBlockBenchmark.common.avg_setBlock += averageData.RandomBlockBenchmark.common.avg_setBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));

		totalAverage.SequentialBlockBenchmark.all.avg_getBlock += averageData.SequentialBlockBenchmark.all.avg_getBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.SequentialBlockBenchmark.all.avg_setBlock += averageData.SequentialBlockBenchmark.all.avg_setBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.SequentialBlockBenchmark.common.avg_getBlock += averageData.SequentialBlockBenchmark.common.avg_getBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));
		totalAverage.SequentialBlockBenchmark.common.avg_setBlock += averageData.SequentialBlockBenchmark.common.avg_setBlock / (double)(sizeof(TEST_AMOUNTS) / sizeof(int));

		totalAverage.RandomBlockBenchmark.tests = averageData.RandomBlockBenchmark.tests;
		totalAverage.SequentialBlockBenchmark.tests = averageData.SequentialBlockBenchmark.tests;
	}

	std::cout << "\n\n";

	std::cout << "========================\n";
	std::cout << "=     TEST RESULTS     =\n";
	std::cout << "========================\n";

	std::cout << totalAverage.toString();

	system("pause");

	return 0;
}