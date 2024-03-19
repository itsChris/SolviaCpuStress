#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cstring>
#include <cmath>
#include <iomanip>

std::mutex io_mutex;

void cpuIntensiveTask(std::atomic<bool>& run, std::vector<unsigned long long>& counts, int threadIndex) {
    while (run) {
        // Perform a CPU-intensive task
        for (unsigned long long i = 0; i < 1000000; ++i) {
            // Increment a counter for benchmarking
            ++counts[threadIndex];
        }
    }
}
void clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void reportStats(std::atomic<bool>& run, const std::vector<unsigned long long>& counts, int intervalMs) {
    int numColumns = std::sqrt(counts.size()); // Adjust based on your preference

    while (run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        clearConsole(); // Clear the console for a fixed table effect

        std::lock_guard<std::mutex> guard(io_mutex);
        std::cout << "CPU Stress tool by solvia.ch\nThread Stats:\n";
        std::cout << "+---------+------------------+\n";
        std::cout << "| Thread  | Iterations       |\n";
        std::cout << "+---------+------------------+\n";

        for (size_t i = 0; i < counts.size(); ++i) {
            std::cout << "| " << std::setw(7) << i + 1 << " | " << std::setw(16) << counts[i] << " |\n";
            if ((i + 1) % numColumns == 0) { // Adjust column formatting based on the number of threads
                std::cout << "+---------+------------------+\n";
            }
        }
        if (counts.size() % numColumns != 0) {
            std::cout << "+---------+------------------+\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "CPU Stress tool by solvia.ch\n";

    // Parse arguments
    if (argc != 3 || std::strcmp(argv[1], "--run-for-seconds") != 0) {
        std::cout << "Usage: " << argv[0] << " --run-for-seconds <seconds>\n";
        return 1;
    }
    int duration = std::stoi(argv[2]);

    unsigned numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::vector<unsigned long long> counts(numThreads, 0);
    std::atomic<bool> run{ true };

    // Start the CPU-intensive tasks on all available threads
    for (unsigned i = 0; i < numThreads; ++i) {
        threads.emplace_back(cpuIntensiveTask, std::ref(run), std::ref(counts), i);
    }

    // Start a reporting thread
    std::thread reporter(reportStats, std::ref(run), std::cref(counts), 500); // Report every 500 ms

    if (duration > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(duration));
        run = false;
    }
    else {
        std::cout << "Running infinitely. Press Ctrl+C to stop...\n";
        while (run) {} // Infinite loop until interrupted
    }

    for (auto& thread : threads) {
        thread.join();
    }
    reporter.join(); // Ensure the reporter thread is also stopped before exiting

    return 0;
}
