#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstdlib>  // system()

using clk = std::chrono::high_resolution_clock;

// ================== MUNCĂ CPU ==================
void cpu_work(std::uint64_t iterations) {
    volatile double a = 1.23456789;
    volatile double b = 9.87654321;
    for (std::uint64_t i = 0; i < iterations; ++i) {
        a = std::sin(a + i * 0.000001);
        b = std::cos(b - i * 0.000001);
        a = a * b + i * 0.0000001;
    }
}

// ================== TEST MULTITHREADING ==================
void run_multithreading(int numThreads, int durationSeconds, std::ofstream& log) {
    auto start = clk::now();
    auto endTime = start + std::chrono::seconds(durationSeconds);

    auto worker = [endTime]() {
        while (clk::now() < endTime) {
            cpu_work(200000); // cantitate de lucru per buclă
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }

    auto end = clk::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 1;

    // aproximație: dacă ai atâtea thread-uri câte nuclee, ~100% CPU
    double cpuPercent = std::min(100.0, 100.0 * numThreads / double(cores));

    log << "=== Test MULTITHREADING ===\n";
    log << "Numar thread-uri: " << numThreads << "\n";
    log << "Timp executie:    " << seconds << " secunde\n";
    log << "CPU aproximativ:  " << cpuPercent << " %\n\n";

    std::cout << "[Multithreading] Timp: " << seconds
              << " s, CPU ~ " << cpuPercent << "%\n";
}

// ================== MOD WORKER (PENTRU PROCESE) ==================
void run_worker_process(int durationSeconds) {
    auto start = clk::now();
    auto endTime = start + std::chrono::seconds(durationSeconds);

    while (clk::now() < endTime) {
        cpu_work(200000);
    }
}

// ================== TEST MULTIPROCESSING ==================
// pe Windows folosim system() ca sa lansam acelasi exe de mai multe ori
void run_multiprocessing(int numProcesses, int durationSeconds, std::ofstream& log) {
    auto start = clk::now();

    // presupunem ca executabilul se numeste pc_tester.exe
    const std::string exeName = "pc_tester.exe";

    // lansam procese in paralel, fiecare cu "worker <durata>"
    std::vector<std::thread> launchers;
    launchers.reserve(numProcesses);

    for (int i = 0; i < numProcesses; ++i) {
        launchers.emplace_back([&]() {
            std::string cmd = exeName + " worker " + std::to_string(durationSeconds);
            std::system(cmd.c_str());
        });
    }

    // asteptam sa termine toate thread-urile care au lansat procesele
    for (auto& t : launchers) {
        if (t.joinable())
            t.join();
    }

    auto end = clk::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 1;
    double cpuPercent = std::min(100.0, 100.0 * numProcesses / double(cores));

    log << "=== Test MULTIPROCESSING ===\n";
    log << "Numar procese:    " << numProcesses << "\n";
    log << "Timp executie:    " << seconds << " secunde (inclusiv lansare procese)\n";
    log << "CPU aproximativ:  " << cpuPercent << " %\n\n";

    std::cout << "[Multiprocessing] Timp: " << seconds
              << " s, CPU ~ " << cpuPercent << "%\n";
}

// ================== MAIN ==================
int main(int argc, char** argv) {
    // MODE: worker -> doar munceste CPU cateva secunde si iese
    if (argc >= 2 && std::string(argv[1]) == "worker") {
        int durationSeconds = 5;
        if (argc >= 3) {
            durationSeconds = std::stoi(argv[2]);
        }
        run_worker_process(durationSeconds);
        return 0;
    }

    // MODE: normal -> rulam testele si scriem in raport.txt
    std::ofstream log("raport.txt");
    if (!log.is_open()) {
        std::cerr << "Nu pot deschide fisierul raport.txt\n";
        return 1;
    }

    log << "Test de performanta CPU (multithreading si multiprocessing)\n";
    log << "-----------------------------------------------------------\n\n";

    int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 4; // fallback

    int durationSeconds = 5;   // durata fiecarui test

    log << "Numar nuclee detectate: " << cores << "\n";
    log << "Durata fiecarui test:   " << durationSeconds << " secunde\n\n";

    // 1) Test multithreading
    run_multithreading(cores, durationSeconds, log);

    // 2) Test multiprocessing
    run_multiprocessing(cores, durationSeconds, log);

    log << "Sfarsit teste.\n";
    std::cout << "Gata. Rezultatele sunt salvate in raport.txt\n";
    return 0;
}
