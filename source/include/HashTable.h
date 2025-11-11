#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string>
#include <iostream>
#include <vector>
#include "odf_types.hpp" 
using namespace std;

template <typename V>
struct HashNode {
    string key;   
    V* value;          
    HashNode* next;

    HashNode(const string& k, V* val) : key(k), value(val), next(nullptr) {}
};

template <typename V>
class HashTable {
private:
    vector<HashNode<V>*> table;
    size_t capacity;
    size_t size;

    
    size_t hash(const string& key) const {
        size_t h = 0;
        for (char c : key) {
            h = (h * 31 + c) % capacity; 
        }
        return h;
    }

public:

    HashTable(size_t cap = 101) : capacity(cap), size(0) {
        table.resize(capacity, nullptr);
    }

    ~HashTable() {
    
        for (size_t i = 0; i < capacity; ++i) {
            HashNode<V>* node = table[i];
            while (node) {
                HashNode<V>* tmp = node;
                node = node->next;
                delete tmp->value;  
                delete tmp;
            }
        }
    }


    bool insert(const string& key, V* value) {
        size_t idx = hash(key);
        HashNode<V>* node = table[idx];
        while (node) {
            if (node->key == key) 
            return false; 
            node = node->next;
        }
        HashNode<V>* new_node = new HashNode<V>(key, value);
        new_node->next = table[idx];
        table[idx] = new_node;
        ++size;
        return true;
    }

    
    V* get(const string& key) const {
        size_t idx = hash(key);
        HashNode<V>* node = table[idx];
        while (node) {
            if (node->key == key) return node->value;
            node = node->next;
        }
        return nullptr; 
    }

    bool remove(const string& key) {
        size_t idx = hash(key);
        HashNode<V>* node = table[idx];
        HashNode<V>* prev = nullptr;
        while (node) {
            if (node->key == key) {
                if (prev) prev->next = node->next;
                else table[idx] = node->next;
                delete node->value;
                delete node;
                --size;
                return true;
            }
            prev = node;
            node = node->next;
        }
        return false;
    }

    size_t getSize() const
     { return size; }
};

#endif 