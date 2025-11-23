#include "../third_party/httplib.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <atomic>
#include <cstdlib>
#include <numeric>
#include <algorithm>
#include <iomanip>

using namespace std;
using namespace httplib;

struct RequestStats {
    double latency_ms;
    bool success;
};

void client_task(string host, int port, int duration, int workload_type, vector<RequestStats>& stats) {
    Client cli(host, port);
    cli.set_connection_timeout(2); 
    cli.set_read_timeout(2);

    auto start_time = chrono::steady_clock::now();
    
    vector<string> keys;
    for(int i=0; i<1000; i++) keys.push_back("key" + to_string(i));

    while (true) {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - start_time).count() >= duration) break;

        string key = keys[rand() % keys.size()];
        string val = "val_" + key;
        
        auto req_start = chrono::steady_clock::now();
        Result res;

        if (workload_type == 0) { 
            if (rand() % 10 == 0) {
                res = cli.Get(("/put?key=" + key + "&val=" + val).c_str());
            } else {
                res = cli.Get(("/get?key=" + key).c_str());
            }
        } else if (workload_type == 1) { 
            res = cli.Get(("/put?key=" + key + "&val=" + val).c_str());
        } else { 
            if (rand() % 2 == 0) {
                res = cli.Get(("/put?key=" + key + "&val=" + val).c_str());
            } else {
                res = cli.Get(("/get?key=" + key).c_str());
            }
        }

        auto req_end = chrono::steady_clock::now();
        double lat = chrono::duration<double, milli>(req_end - req_start).count();
        
        bool success = (res && res->status == 200);
        stats.push_back({lat, success});
    }
}

int main(int argc, char** argv) {
    if (argc < 6) {
        cerr << "Usage: ./loadgen <host> <port> <threads> <duration> <workload_type>\n";
        cerr << "Workload 0: Read Heavy (90% Read, 10% Write)\n";
        cerr << "Workload 1: Write Heavy (100% Write)\n";
        cerr << "Workload 2: Balanced (50% Read, 50% Write)\n";
        return 1;
    }

    string host = argv[1];
    int port = stoi(argv[2]);
    int num_threads = stoi(argv[3]);
    int duration = stoi(argv[4]);
    int workload_type = stoi(argv[5]);

    cout << "Starting Load Test with " << num_threads << " threads for " << duration << "s..." << endl;

    vector<thread> threads;
    vector<vector<RequestStats>> all_stats(num_threads);

    auto start_global = chrono::steady_clock::now();

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(client_task, host, port, duration, workload_type, ref(all_stats[i]));
    }

    for (auto& t : threads) t.join();


    vector<double> latencies;
    long long total_success = 0;
    for (const auto& thread_stat : all_stats) {
        size_t warmup_skip = thread_stat.size() * 0; 
        for (size_t i = warmup_skip; i < thread_stat.size(); i++) {
            if (thread_stat[i].success) {
                total_success++;
                latencies.push_back(thread_stat[i].latency_ms);
            }
        }
    }

    if (latencies.empty()) {
        cout << "No successful requests recorded (check server connectivity)." << endl;
        return 0;
    }

    sort(latencies.begin(), latencies.end());

    double sum = accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg_lat = sum / latencies.size();
    double p99_lat = latencies[latencies.size() * 0.99];
    double p50_lat = latencies[latencies.size() * 0.50];

    double effective_duration = duration * 0.8; 
    double throughput = total_success / effective_duration;

    cout << "=== Results (Steady State) ===\n";
    cout << "Concurrency (Load): " << num_threads << "\n";
    cout << "Throughput:         " << fixed << setprecision(2) << throughput << " req/s\n";
    cout << "Avg Latency:        " << avg_lat << " ms\n";
    cout << "P50 Latency:        " << p50_lat << " ms\n";
    cout << "P99 Latency:        " << p99_lat << " ms\n";
}