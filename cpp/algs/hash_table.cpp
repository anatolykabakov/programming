#include <cassert>
#include <iostream>

class HashTableEntry {
public:
    HashTableEntry *next;
    HashTableEntry(int k, int v) {
        key_ = k;
        value_ = v;
        next = nullptr;
    }
    int get_key() { return key_; }
    int get_value() { return value_; }
    void change_value(int value) { value_ = value; }

private:
    int key_;
    int value_;
};
/**
 * @brief HashTable is data structure which is used to store key value pairs.
 * @ref
 * https://www.tutorialspoint.com/cplusplus-program-to-implement-hash-tables-chaining-with-list-heads
 *
 */
class HashTable {
public:
    HashTable(int size) : size_(size) {
        array_ = new HashTableEntry *[size_];
        for (int i = 0; i < size_; ++i) {
            array_[i] = nullptr;
        }
    }

    void insert(int k, int v) {
        int index = hash_function_(k);
        if (array_[index] == nullptr) {
            array_[index] = new HashTableEntry(k, v);
        } else {
            HashTableEntry *el = array_[index];
            while (el->next) {
                el = el->next;
            }
            if (el->get_key() == k) {
                el->change_value(v);
            } else {
                el->next = new HashTableEntry(k, v);
            }
        }
    }

    HashTableEntry *get(int k) {
        int index = hash_function_(k);

        HashTableEntry *el = array_[index];
        while (el && el->get_key() != k) {
            el = el->next;
        }
        return el;  // return nullptr or HashTableEntry*
    }

    void remove(int k) {
        int index = hash_function_(k);
        if (array_[index] != nullptr) {
            HashTableEntry *el = array_[index];
            HashTableEntry *prev = nullptr;
            while (el->next && el->get_key() != k) {
                prev = el;
                el = el->next;
            }

            if (el->get_key() == k) {
                if (prev == nullptr) {
                    HashTableEntry *next = el->next;
                    delete el;
                    array_[index] = next;
                } else {
                    HashTableEntry *next = el->next;
                    delete el;
                    prev->next = next;
                }
            }
        }
    }

    ~HashTable() {
        for (int i = 0; i < size_; ++i) {
            if (array_[i] != nullptr) {
                delete array_[i];
            }
        }
        delete[] array_;
    }

private:
    HashTableEntry **array_;
    int size_;

private:
    int hash_function_(int k) { return k % size_; }
};

int main() {
    HashTable hash_table(1);
    hash_table.insert(1, 2);
    HashTableEntry *p = hash_table.get(1);
    std::cout << "key " << p->get_key() << " value " << p->get_value() << std::endl;
    hash_table.remove(1);
    p = hash_table.get(1);
    assert(p == nullptr);

    return 0;
}
