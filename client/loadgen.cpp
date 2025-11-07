#include "../third_party/httplib.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <atomic>
#include <cstdlib>

using namespace std;
using namespace httplib;

void client_task(string host, int port, int id, int duration, atomic<int>& success) {
    Client cli(host, port);
    auto start = chrono::steady_clock::now();
     while (chrono::duration_cast<chrono::seconds>(
           chrono::steady_clock::now() - start).count() < duration) {
    string key = "key" + to_string(rand() % 100);
    string val = "val" + to_string(rand() % 100);

    auto res = cli.Get(("/put?key=" + key + "&val=" + val).c_str());
    if (res && res->status == 200) success++;

    auto getres = cli.Get(("/get?key=" + key).c_str());
    if (getres && getres->status == 200) success++;
}

}

int main(int argc, char** argv) {
    if (argc < 5) {
        cerr << "Usage: ./loadgen <host> <port> <num_threads> <duration>\n";
        return 1;
    }

    string host = argv[1];
    int port = stoi(argv[2]);
    int num_threads = stoi(argv[3]);
    int duration = stoi(argv[4]);
    atomic<int> success{0};

    vector<thread> threads;
    auto start = chrono::steady_clock::now();

    for (int i = 0; i < num_threads; i++)
        threads.emplace_back(client_task, host, port, i, duration, ref(success));

    for (auto& t : threads) t.join();

    auto elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
    cout << "=== Load test result ===\n";
    cout << "Successful responses: " << success << "\n";
    cout << "Throughput (req/s): " << success / (double)elapsed << "\n";
}
