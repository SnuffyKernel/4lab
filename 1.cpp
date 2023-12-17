#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <benchmark/benchmark.h>

const int threadCount = 5;
const int iterations = 100;

std::mutex mtx;
std::binary_semaphore sem(1);
std::atomic_flag spinLock = ATOMIC_FLAG_INIT;
std::vector<std::thread> threads;

void raceWithMutex() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < iterations; ++i) {
        std::cout << static_cast<char>('A' + rand() % 26);
        benchmark::DoNotOptimize(std::cout << static_cast<char>('A' + rand() % 26));
    }
}

void raceWithSemaphore() {
    sem.acquire();
    for (int i = 0; i < iterations; ++i) {
        std::cout << static_cast<char>('A' + rand() % 26);
        benchmark::DoNotOptimize(std::cout << static_cast<char>('A' + rand() % 26));
    }
    sem.release();
}

void raceWithSpinLock() {
    while (spinLock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    for (int i = 0; i < iterations; ++i) {
        std::cout << static_cast<char>('A' + rand() % 26);
        benchmark::DoNotOptimize(std::cout << static_cast<char>('A' + rand() % 26));
    }
    spinLock.clear(std::memory_order_release);
}

void raceWithSpinWait() {
    std::atomic_flag spinFlag = ATOMIC_FLAG_INIT;
    while (spinFlag.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    for (int i = 0; i < iterations; ++i) {
        std::cout << static_cast<char>('A' + rand() % 26);
        benchmark::DoNotOptimize(std::cout << static_cast<char>('A' + rand() % 26));
    }
    spinFlag.clear(std::memory_order_release);
}

void raceWithMonitor() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < iterations; ++i) {
        std::cout << static_cast<char>('A' + rand() % 26);
        benchmark::DoNotOptimize(std::cout << static_cast<char>('A' + rand() % 26));
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
    std::cout << "\nTime with Mutex: " << std::chrono::duration_cast<std::chrono::milliseconds>(endMutex - startMutex).count() << " ms\n\n";

    threads.clear();

    auto startSemaphore = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(raceWithSemaphore);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    auto endSemaphore = std::chrono::high_resolution_clock::now();
    std::cout << "\nTime with Semaphore: " << std::chrono::duration_cast<std::chrono::milliseconds>(endSemaphore - startSemaphore).count() << " ms\n\n";

    threads.clear();

    auto startSpinLock = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(raceWithSpinLock);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    auto endSpinLock = std::chrono::high_resolution_clock::now();
    std::cout << "\nTime with SpinLock: " << std::chrono::duration_cast<std::chrono::milliseconds>(endSpinLock - startSpinLock).count() << " ms\n\n";
    threads.clear();

    auto startSpinWait = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(raceWithSpinWait);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    auto endSpinWait = std::chrono::high_resolution_clock::now();
    std::cout << "\nTime with SpinWait: " << std::chrono::duration_cast<std::chrono::milliseconds>(endSpinWait - startSpinWait).count() << " ms\n\n";

    threads.clear();

    auto startMonitor = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(raceWithMonitor);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    auto endMonitor = std::chrono::high_resolution_clock::now();
    std::cout << "\nTime with Monitor: " << std::chrono::duration_cast<std::chrono::milliseconds>(endMonitor - startMonitor).count() << " ms\n\n";

    return 0;
}
