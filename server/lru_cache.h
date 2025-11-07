#pragma once
#include <unordered_map>
#include <list>
#include <mutex>

template<typename K, typename V>
class LRUCache {
    size_t capacity;
    std::list<std::pair<K, V>> items;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map;
    std::mutex mtx;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    bool get(const K& key, V& value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it == map.end()) return false;
        items.splice(items.begin(), items, it->second);
        value = it->second->second;
        return true;
    }

    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) {
            items.erase(it->second);
        } else if (items.size() >= capacity) {
            auto last = items.back();
            map.erase(last.first);
            items.pop_back();
        }
        items.emplace_front(key, value);
        map[key] = items.begin();
    }

    void del(const K& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) {
            items.erase(it->second);
            map.erase(it);
        }
    }
};
