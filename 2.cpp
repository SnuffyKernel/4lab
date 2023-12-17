#include <iostream>
#include <vector>
#include<string>
#include <thread>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <mutex>

struct Apartment {
    std::string address;
    int rooms;
    double cost;
    double distanceToMetro;
};

std::vector<Apartment> generateApartments(int size) {
    std::vector<Apartment> apartments;
    for (int i = 0; i < size; ++i) {
        
        apartments.push_back({ "Address" + std::to_string(i), rand() % 5 + 1, static_cast<double>(rand() % 1000) + 500, static_cast<double>(rand() % 100) / 10 });
    }
    return apartments;
}

void printApartments(const std::vector<Apartment>& apartments) {
    for (const auto& apartment : apartments) {
        std::cout << "Address: " << apartment.address << ", Rooms: " << apartment.rooms
            << ", Cost: " << apartment.cost << ", Distance to Metro: " << apartment.distanceToMetro << " km\n";
    }
}

std::vector<Apartment> findApartments(const std::vector<Apartment>& apartments, double& averageCost) {
    std::vector<Apartment> result;
    double totalCost = 0.0;
    int count = 0;

    for (const auto& apartment : apartments) {
        if (apartment.distanceToMetro < 1.0) {
            totalCost += apartment.cost;
            ++count;
            result.push_back(apartment);
        }
    }

    averageCost = (count > 0) ? totalCost / count : 0.0;
    return result;
}

void printBelowAverageCost(const std::vector<Apartment>& apartments, double averageCost) {
    std::cout << "\nApartments with cost below average (" << averageCost << "):\n";
    for (const auto& apartment : apartments) {
        if (apartment.cost < averageCost) {
            std::cout << "Address: " << apartment.address << ", Rooms: " << apartment.rooms
                << ", Cost: " << apartment.cost << ", Distance to Metro: " << apartment.distanceToMetro << " km\n";
        }
    }
}

void processInParallel(const std::vector<Apartment>& apartments, double& averageCost, std::mutex& mutex) {
    std::vector<Apartment> result;
    double totalCost = 0.0;
    int count = 0;

    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, numThreads, &apartments, &result, &totalCost, &count, &mutex]() {
            for (size_t j = i; j < apartments.size(); j += numThreads) {
                const auto& apartment = apartments[j];
                if (apartment.distanceToMetro < 1.0) {
                    std::lock_guard<std::mutex> lock(mutex);
                    totalCost += apartment.cost;
                    ++count;
                    result.push_back(apartment);
                }
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    averageCost = (count > 0) ? totalCost / count : 0.0;

    printApartments(result);
    printBelowAverageCost(result, averageCost);
}

int main() {
    const int dataSize = 1000;
    std::vector<Apartment> apartments = generateApartments(dataSize);

    std::cout << "Data Size: " << dataSize << std::endl;

    double averageCostWithoutThreads = 0.0;
    auto startWithoutThreads = std::chrono::high_resolution_clock::now();
    auto resultWithoutThreads = findApartments(apartments, averageCostWithoutThreads);
    auto endWithoutThreads = std::chrono::high_resolution_clock::now();

    std::cout << "\nTime without threads: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(endWithoutThreads - startWithoutThreads).count()
        << " ms\n";

    double averageCostWithThreads = 0.0;
    auto startWithThreads = std::chrono::high_resolution_clock::now();
    std::mutex mutex;
    processInParallel(apartments, averageCostWithThreads, mutex);
    auto endWithThreads = std::chrono::high_resolution_clock::now();

    std::cout << "\nTime with threads: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(endWithThreads - startWithThreads).count()
        << " ms\n";

    return 0;
}
