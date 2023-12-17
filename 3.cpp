#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

std::mutex mtx;
std::condition_variable cv_reader, cv_writer;
int readers_count = 0;
bool writing = false;
std::queue<int> data;

void reader(int id) {
    std::unique_lock<std::mutex> lock(mtx);
    cv_reader.wait(lock, [] { return !writing; });
    ++readers_count;
    lock.unlock();

    std::cout << "Reader " << id << " is reading data: ";
    lock.lock();
    while (!data.empty()) {
        std::cout << data.front() << " ";
        data.pop();
    }
    std::cout << std::endl;
    --readers_count;

    if (readers_count == 0) {
        cv_writer.notify_one();
    }
}

void writer(int id) {
    std::unique_lock<std::mutex> lock(mtx);
    cv_writer.wait(lock, [id] { return readers_count == 0 && !writing; });
    writing = true;
    lock.unlock();

    std::cout << "Writer " << id << " is writing data" << std::endl;
    data.push(id);

    lock.lock();
    writing = false;
    cv_writer.notify_one();
    cv_reader.notify_all();
}

int main() {
    const int num_readers = 3;
    const int num_writers = 2;

    std::thread readers[num_readers];
    std::thread writers[num_writers];

    for (int i = 0; i < num_readers; ++i) {
        readers[i] = std::thread(reader, i + 1);
    }

    for (int i = 0; i < num_writers; ++i) {
        writers[i] = std::thread(writer, i + 1);
    }

    for (int i = 0; i < num_readers; ++i) {
        readers[i].join();
    }

    for (int i = 0; i < num_writers; ++i) {
        writers[i].join();
    }

    return 0;
}
