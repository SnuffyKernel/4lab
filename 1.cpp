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
	std::random_device rd;
	std::default_random_engine eng(rd());
	std::uniform_int_distribution<> distr(' ', '~');
	return  static_cast<char>(distr(eng));
}

void runBenchmark() {
	benchmark::DoNotOptimize(std::cout << randomAscii());
}

auto time() {
	return std::chrono::high_resolution_clock::now();
}

uint64_t timeCount(auto end, auto start) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void iterationsLoop() {
	for (int i = 0; i < iterations; ++i) {
		std::cout << randomAscii();
		runBenchmark();
	}
}

void startThread(auto race, std::string primitive) {
	auto start = time();
	for (int i = 0; i < threadCount; ++i) {
		threads.emplace_back(race);
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto end = time();
	std::cout << "\nTime with " << primitive << ": " << timeCount(end - start) << " ms\n\n";

	threads.clear();
}

void raceWithMutex() {
	std::lock_guard<std::mutex> lock(mtx);
	iterationsLoop();
}

void raceWithSemaphore() {
	sem.acquire();
	iterationsLoop();
	sem.release();
}

void raceWithBarrier() {
	iterationsLoop();
	barrier.arrive_and_wait();
}

void raceWithSpinLock() {
	while (spinLock.test_and_set(std::memory_order_acquire)) {
		std::this_thread::yield();
	}
	iterationsLoop();
	spinLock.clear(std::memory_order_release);
}

void raceWithSpinWait() {
	std::atomic_flag spinFlag = ATOMIC_FLAG_INIT;
	while (spinFlag.test_and_set(std::memory_order_acquire)) {
		std::this_thread::yield();
	}
	iterationsLoop();
	spinFlag.clear(std::memory_order_release);
}

void raceWithMonitor() {
	std::lock_guard<std::mutex> lock(mtx);
	iterationsLoop();
}

BENCHMARK(raceWithMutex);
BENCHMARK(raceWithSemaphore);
BENCHMARK(raceWithBarrier);
BENCHMARK(raceWithSpinLock);
BENCHMARK(raceWithSpinWait);
BENCHMARK(raceWithMonitor);

BENCHMARK_MAIN();

int main() {

	startThread(raceWithMutex, "Mutex");
	startThread(raceWithSemaphore, "Semaphore");
	startThread(raceWithBarrier, "Barrier");
	startThread(raceWithSpinLock, "SpinLock");
	startThread(raceWithSpinWait, "SpinWait");
	startThread(raceWithMonitor, "Monitor");

	return 0;
}
