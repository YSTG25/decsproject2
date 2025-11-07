#pragma once
#include <fstream>
#include <unordered_map>
#include <string>
#include <mutex>
#include <iostream>

class DB {
    std::string filename;
    std::unordered_map<std::string, std::string> kv;
    std::mutex mtx;

public:
    DB(const std::string& fname) : filename(fname) {
        std::ifstream file(filename);
        std::string k, v;
        while (file >> k >> v) kv[k] = v;
    }

    void put(const std::string& k, const std::string& v) {
        std::lock_guard<std::mutex> lock(mtx);
        kv[k] = v;

        std::ofstream file(filename, std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "[DB ERROR] Failed to open file for write\n";
            return;
        }

        for (auto& p : kv) {
            file << p.first << " " << p.second << "\n";
        }
    }

    bool get(const std::string& k, std::string& v) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = kv.find(k);
        if (it == kv.end()) return false;
        v = it->second;
        return true;
    }

    void del(const std::string& k) {
        std::lock_guard<std::mutex> lock(mtx);
        kv.erase(k);

        std::ofstream file(filename, std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "[DB ERROR] Failed to open file for write\n";
            return;
        }

        for (auto& p : kv)
            file << p.first << " " << p.second << "\n";
    }
};
