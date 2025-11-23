#include "lru_cache.h"
#include "thread_pool.h"
#include "db.h"
#include "../third_party/httplib.h"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

using namespace std;
using namespace httplib;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: ./kv_server <port> <cache_capacity>\n";
        return 1;
    }

    int port = stoi(argv[1]);
    int cache_capacity = stoi(argv[2]);
    string conn_str = "dbname=kv_store user=myuser password=ystg1234 host=localhost port=5433";
    auto db = std::make_shared<DB>(conn_str, 4, false);
    
    auto cache = std::make_shared<LRUCache<string, string>>(cache_capacity);
    auto pool = std::make_shared<custom::ThreadPool>(4);

    Server svr;

    svr.Get("/put", [db, cache, pool](const Request& req, Response& res) {
        if (!req.has_param("key") || !req.has_param("val")) {
            res.status = 400;
            return;
        }
        auto key = req.get_param_value("key");
        auto val = req.get_param_value("val");

        pool->enqueue([db, cache, key, val]() {
            db->put(key, val);
            cache->put(key, val);
        });

        res.set_content("OK", "text/plain");
    });

    svr.Get("/get", [db, cache](const Request& req, Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            return;
        }
        auto key = req.get_param_value("key");
        std::string val;

        if (cache->get(key, val)) {
            res.set_content(val, "text/plain");
        } else if (db->get(key, val)) {
            cache->put(key, val);
            res.set_content(val, "text/plain");
        } else {
            res.status = 404;
            res.set_content("Not Found", "text/plain");
        }
    });

    svr.Get("/del", [db, cache](const Request& req, Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            return;
        }
        auto key = req.get_param_value("key");
        db->del(key);
        cache->del(key);
        res.set_content("Deleted", "text/plain");
    });
    
    svr.Get("/dump", [db](const Request&, Response& res) {
        std::ostringstream oss;
        oss << "=== Database Dump (Postgres) ===\n";
        auto all_data = db->getAll();
        for (const auto& p : all_data) {
            oss << p.first << " => " << p.second << "\n";
        }
        res.set_content(oss.str(), "text/plain");
    });

    cout << "Server running on port " << port << " with PostgreSQL backend." << endl;
    svr.listen("0.0.0.0", port);
    return 0;
}