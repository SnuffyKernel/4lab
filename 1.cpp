#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <barrier>
#include <benchmark/benchmark.h>

const int threadCount = 5;
const int iterations = 100;

std::mutex mtx;
std::binary_semaphore sem(1);
std::barrier barrier(threadCount);
std::atomic_flag spinLock = ATOMIC_FLAG_INIT;
std::vector<std::thread> threads;

char randomAscii() {
	return  static_cast<char>('A' + rand() % 26);
}

void runBenchmark() {
	benchmark::DoNotOptimize(std::cout << randomAscii());
}

uint64_t timeCount(auto end, auto start) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void raceWithMutex() {
	std::lock_guard<std::mutex> lock(mtx);
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
}

void raceWithSemaphore() {
	sem.acquire();
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
	sem.release();
}

void raceWithBarrier() {
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
	barrier.arrive_and_wait();
}

void raceWithSpinLock() {
	while (spinLock.test_and_set(std::memory_order_acquire)) {
		std::this_thread::yield();
	}
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
	spinLock.clear(std::memory_order_release);
}

void raceWithSpinWait() {
	std::atomic_flag spinFlag = ATOMIC_FLAG_INIT;
	while (spinFlag.test_and_set(std::memory_order_acquire)) {
		std::this_thread::yield();
	}
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
	spinFlag.clear(std::memory_order_release);
}

void raceWithMonitor() {
	std::lock_guard<std::mutex> lock(mtx);
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
}

BENCHMARK(raceWithMutex);
BENCHMARK(raceWithSemaphore);
BENCHMARK(raceWithSpinLock);
BENCHMARK(raceWithSpinWait);
BENCHMARK(raceWithMonitor);

BENCHMARK_MAIN();

int main() {
	auto startMutex = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithMutex);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endMutex = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with Mutex: " << timeCount(endMutex - startMutex) << " ms\n\n";

	threads.clear();

	auto startSemaphore = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithSemaphore);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endSemaphore = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with Semaphore: " << timeCount(endSemaphore - startSemaphore) << " ms\n\n";

	threads.clear();

	auto startSpinLock = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithSpinLock);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endSpinLock = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with SpinLock: " << timeCount(endSpinLock - startSpinLock) << " ms\n\n";
	threads.clear();

	auto startSpinWait = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithSpinWait);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endSpinWait = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with SpinWait: " << timeCount(endSpinWait - startSpinWait) << " ms\n\n";

	threads.clear();

	auto startMonitor = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithMonitor);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endMonitor = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with Monitor: " << timeCount(endMonitor - startMonitor) << " ms\n\n";

	auto startBarrier = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(raceWithBarrier);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto endBarrier = std::chrono::high_resolution_clock::now();
	std::cout << "\nTime with Barrier: " << timeCount(endBarrier - startBarrier) << " ms\n";


	return 0;
}
