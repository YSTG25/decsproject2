#pragma once
#include <string>
#include <mutex>
#include <iostream>
#include <pqxx/pqxx>
#include <queue>
#include <memory>
#include <condition_variable>

class DB {
    std::string conn_string;
    std::queue<std::shared_ptr<pqxx::connection>> connection_pool;
    std::mutex pool_mtx;
    std::condition_variable pool_cv;
    
    bool simulate_io_delay; 

public:
    DB(const std::string& conn_str, int pool_size, bool io_delay) 
        : conn_string(conn_str), simulate_io_delay(io_delay) {
        
        for (int i = 0; i < pool_size; ++i) {
            try {
                auto C = std::make_shared<pqxx::connection>(conn_string);
                if (C->is_open()) {
                    connection_pool.push(C);
                } else {
                    std::cerr << "[DB ERROR] Can't open database" << std::endl;
                }
            } catch (const std::exception &e) {
                std::cerr << "[DB EXCEPTION] " << e.what() << std::endl;
            }
        }
    }

    std::shared_ptr<pqxx::connection> getConnection() {
        std::unique_lock<std::mutex> lock(pool_mtx);
        pool_cv.wait(lock, [this] { return !connection_pool.empty(); });
        
        auto conn = connection_pool.front();
        connection_pool.pop();
        return conn;
    }
    void returnConnection(std::shared_ptr<pqxx::connection> conn) {
        std::lock_guard<std::mutex> lock(pool_mtx);
        connection_pool.push(conn);
        pool_cv.notify_one();
    }

    void put(const std::string& k, const std::string& v) {
        auto C = getConnection();
        try {
            pqxx::work W(*C);
            W.exec_params(
                "INSERT INTO store (key_name, val_content) VALUES ($1, $2) "
                "ON CONFLICT (key_name) DO UPDATE SET val_content = $2",
                k, v
            );
            W.commit();
        } catch (const std::exception &e) {
            std::cerr << "[PUT ERROR] " << e.what() << std::endl;
        }
        returnConnection(C);
    }

    bool get(const std::string& k, std::string& v) {
        auto C = getConnection();
        bool found = false;
        try {
            pqxx::nontransaction N(*C);
            pqxx::result R = N.exec_params("SELECT val_content FROM store WHERE key_name = $1", k);
            
            if (!R.empty()) {
                v = R[0][0].c_str();
                found = true;
            }
        } catch (const std::exception &e) {
            std::cerr << "[GET ERROR] " << e.what() << std::endl;
        }
        returnConnection(C);
        return found;
    }

    void del(const std::string& k) {
        auto C = getConnection();
        try {
            pqxx::work W(*C);
            W.exec_params("DELETE FROM store WHERE key_name = $1", k);
            W.commit();
        } catch (const std::exception &e) {
            std::cerr << "[DEL ERROR] " << e.what() << std::endl;
        }
        returnConnection(C);
    }
    
    std::vector<std::pair<std::string, std::string>> getAll() {
        auto C = getConnection();
        std::vector<std::pair<std::string, std::string>> result;
        try {
            pqxx::nontransaction N(*C);
            pqxx::result R = N.exec("SELECT key_name, val_content FROM store");
            for (auto row : R) {
                result.push_back({row[0].c_str(), row[1].c_str()});
            }
        } catch (...) {}
        returnConnection(C);
        return result;
    }
};