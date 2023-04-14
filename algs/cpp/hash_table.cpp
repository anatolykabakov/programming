#include <iostream>

const uint CAPACITY = 5000;

uint hash_function(char* str) {
  unsigned long i = 0;
  for (int j=0; str[j]; j++)
      i += str[j];
  return i % CAPACITY;
}

typedef struct Ht_item Ht_item;


struct Ht_item {
  char* key;
  char* value;
};

typedef struct HashTable HashTable;

// Define the Hash Table here
struct HashTable {
    // Contains an array of pointers
    // to items
    Ht_item** items;
    int size;
    int count;
};


int main() {

  std::cout << hash_function("Hel") << std::endl;
  std::cout << hash_function("Cau") << std::endl;

  return 0;
}