// INFO:
// This file is not part of BitVector.hpp library!
// It's a separate file containing quickly written classes for Unit Tests and Benchmarking

#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

void set_console_color(uint8_t color) {
#ifdef _WIN32
	// Windows: Use the native API which matches your bitmask format perfectly
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
#else
	// POSIX (Linux/Mac): Map Windows 4-bit color to ANSI
	// Windows format: bit 0:Blue, 1:Green, 2:Red, 3:Intensity

	auto get_ansi = [](uint8_t win_color, bool is_background) -> int {
		uint8_t intensity = (win_color & 8) >> 3;
		uint8_t rgb = win_color & 7;

		// Reorder Windows BGR to ANSI RGB
		// Windows: 1=B, 2=G, 4=R | ANSI: 1=R, 2=G, 4=B
		static const int mapping[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
		int ansi_base = mapping[rgb];

		if (is_background) {
			return (intensity ? 100 : 40) + ansi_base;
		}
		else {
			return (intensity ? 90 : 30) + ansi_base;
		}
		};

	int fg = get_ansi(color & 0x0F, false);
	int bg = get_ansi((color & 0xF0) >> 4, true);

	std::printf("\033[%d;%dm", fg, bg);
#endif
}

class UnitTest;
class UnitTestCollection;

struct UnitTestStats {
	int totalTests = 0;
	int passes = 0, failures = 0;
	int expectedFailures = 0, unexpectedPasses = 0;
	int totalPasses = 0; int totalFailures = 0;
};

class UnitTestCollection {
private:

	std::vector<UnitTest> tests;

	void runTests(bool expectingSuccess, UnitTestStats& stats, int nested);

public:

	UnitTestCollection(const std::vector<UnitTest>& _tests) : tests(_tests) {}
	void runTests();

};

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#else
// Helper to get result type in C++14 (since std::invoke_result is C++17)
template <typename F, typename... Args>
using result_of_t = typename std::result_of<F(Args...)>::type;
#endif

class UnitTest {
private:

	std::string name;
	std::function<bool(std::string& failureReason)> test;
	std::string failureReason = "";
	UnitTestCollection childTests;

	void setTest(std::function<bool()> _test) {
		test = std::function<bool(std::string&)>([_test](std::string& failureReason) -> bool {
			try {
				bool success = _test();
				if (!success) failureReason = "Test Failed";
				return success;
			}
			catch (std::runtime_error e) {
				failureReason = std::string("std::runtime_error: ") + e.what();
				return false;
			}
			catch (std::exception e) {
				failureReason = std::string("std::exception: ") + e.what();
				return false;
			}
			catch (...) {
				failureReason = "Exception: Unknown Exception";
				return false;
			}
			failureReason = "Unknown Error!";
			return false;
			});
	};

	friend UnitTestCollection;

public:

	UnitTest() = delete;

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
	template <typename Func>
	UnitTest(const std::string& _name, Func _test, UnitTestCollection _childTests = { {} }) : name(_name), childTests(_childTests) {
		if constexpr (std::is_same_v<std::invoke_result_t<Func>, bool>) {
			setTest(std::function<bool()>(_test));
		}
		else {
			setTest(std::function<bool()>([_test]() { _test(); return true; }));
		}
	}
#else
	// Overload for functions returning bool
	template <typename Func,
		typename std::enable_if<
		std::is_same<result_of_t<Func>, bool>::value,
		int>::type = 0>
	UnitTest(const std::string& _name, Func _test, UnitTestCollection _childTests = { {} })
		: name(_name), childTests(_childTests)
	{
		setTest(std::function<bool()>(_test));
	}

	// Overload for functions returning void (or anything else)
	template <typename Func,
		typename std::enable_if<
		!std::is_same<result_of_t<Func>, bool>::value,
		int>::type = 0>
	UnitTest(const std::string& _name, Func _test, UnitTestCollection _childTests = { {} })
		: name(_name), childTests(_childTests)
	{
		setTest(std::function<bool()>([_test]() {
			_test();
			return true;
			}));
	}
#endif

};



void UnitTestCollection::runTests(bool expectingSuccess, UnitTestStats& stats, int nested) {

	for (auto& test : tests) {
		for (int i = 0; i < nested; i++) std::cout << "  ";

		set_console_color(0xB); std::cout << "[Test] Running test '" << std::flush;
		set_console_color(0xF); std::cout << test.name << std::flush;
		set_console_color(0xB); std::cout << "'..." << std::flush;

		if (!expectingSuccess) {
			set_console_color(0x3); std::cout << " (expecting failure)" << std::flush;
		}

		// Run test
		std::string failureReason = "";
		bool passed = test.test(failureReason);
		stats.totalTests++;

		// Console formatting stuff
		set_console_color(0x7);
		std::cout << "\r";
		for (int i = 0; i < nested; i++) std::cout << "  ";
		for (int i = 0; i < 45 + test.name.length(); i++) std::cout << " ";
		std::cout << "\r";
		for (int i = 0; i < nested; i++) std::cout << "  ";

		// Print result
		if (expectingSuccess) {
			if (passed) {
				stats.passes++;
				stats.totalPasses++;

				set_console_color(0xA); std::cout << "[Test] '" << std::flush;
				set_console_color(0x2); std::cout << test.name << std::flush;
				set_console_color(0xA); std::cout << "' Passed!\n" << std::flush;
			}
			else {
				stats.failures++;
				stats.totalFailures++;

				set_console_color(0x04); std::cout << "[Test] '" << std::flush;
				set_console_color(0x0C); std::cout << test.name << std::flush;
				set_console_color(0x04); std::cout << "' " << std::flush;
				set_console_color(0x4F); std::cout << "FAILED" << std::flush;

				if (failureReason != "" && failureReason != "Test Failed") {
					set_console_color(0x4); std::cout << " Reason: " << std::flush;
					set_console_color(0xC); std::cout << failureReason << "\n" << std::flush;
				}
				else std::cout << std::endl;
			}
		}
		else {
			if (passed) {
				stats.unexpectedPasses++;
				stats.totalPasses++;

				set_console_color(0x6); std::cout << "[Test] '" << std::flush;
				set_console_color(0x6); std::cout << test.name << std::flush;
				set_console_color(0x6); std::cout << "' Unexpectedly Passed!\n" << std::flush;
			}
			else {
				stats.expectedFailures++;
				stats.totalFailures++;

				set_console_color(0xD); std::cout << "[Test] '" << std::flush;
				set_console_color(0x5); std::cout << test.name << std::flush;
				set_console_color(0xD); std::cout << "' Expectedly Failed!" << std::flush;

				if (failureReason != "" && failureReason != "Test Failed") {
					set_console_color(0x5); std::cout << " Reason: " << std::flush;
					set_console_color(0xD); std::cout << failureReason << "\n" << std::flush;
				}
				else std::cout << std::endl;
			}
		}
	
		// Recursively run child tests
		test.childTests.runTests(expectingSuccess && passed, stats, nested + 1);
	}

}

void UnitTestCollection::runTests() {
	set_console_color(0x7); std::cout << "Starting Tests...\n\n" << std::flush;

	UnitTestStats stats;
	runTests(true, stats, 0);

	std::cout << std::endl;

	set_console_color(stats.totalFailures == 0 ? 0xA : 0xC); std::cout << "====================" << std::flush;
	set_console_color(0xF); std::cout << " Results " << std::flush;
	set_console_color(stats.totalFailures == 0 ? 0xA : 0xC); std::cout << "====================\n" << std::flush;

	auto numToStr = [](int num) -> std::string {
		std::string str = std::to_string(num);
		while (str.length() < 3)
			str += " ";
		return str;
		};

	set_console_color(0x08); std::cout << " " << numToStr(stats.totalTests)			<< " Tests Executed " << std::endl; set_console_color(0x7);
	set_console_color(0x0A); std::cout << " " << numToStr(stats.totalPasses)				<< " Passes         " << std::endl; set_console_color(0x7);
	set_console_color(stats.totalFailures == 0 ? 0x8 : 0x4F); std::cout << " " << numToStr(stats.totalFailures)			<< " Failures       " << std::endl; set_console_color(0x7);

	if (stats.expectedFailures > 0 || stats.unexpectedPasses > 0) {
		set_console_color(0x8); std::cout << "-------------------------------------------------" << std::endl; set_console_color(0x7);
	}

	if (stats.expectedFailures > 0) {
		set_console_color(0x0D); std::cout << " " << numToStr(stats.expectedFailures) << " of those tests were Expected Failures " << std::endl; set_console_color(0x7);
	}
	if (stats.unexpectedPasses > 0) {
		set_console_color(0x06); std::cout << " " << numToStr(stats.unexpectedPasses) << " of those tests were Unexpected Passes " << std::endl; set_console_color(0x7);
	}

	std::cout << std::endl;

	set_console_color(0x7);
}
