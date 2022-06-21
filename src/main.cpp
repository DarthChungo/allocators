#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>

class InlineHeaderAllocator {
 public:
  InlineHeaderAllocator(void* start, std::size_t size) : start(start), size(size) { std::memset(start, 0, size); }
  ~InlineHeaderAllocator() {}

  template <typename T>
  T* malloc() {
    return nullptr;
  }

  template <typename T>
  void free(T* record) {
    assert(record && "Invalid address nullptr");
  }

  void print_records() {
    void* current = start;

    while (*(size_t*)current != 0) {
      std::cout << "Record: header ptr=" << current << ", data ptr=" << ((size_t*)current + 1)
                << ", data (as byte)=" << (int)*((uint8_t*)current + sizeof(size_t)) << "\n";
      current = (void*)((uint8_t*)current + *(size_t*)current + sizeof(size_t));
    }
  }

 public:
  void*       start = nullptr;
  std::size_t size  = 0;
};

int main() {
  InlineHeaderAllocator allocator(std::malloc(1024), 1024);

  // manually test writing some records
  *((uint8_t*)allocator.start + (0 * sizeof(size_t)) + 0) = 1;
  *((uint8_t*)allocator.start + (1 * sizeof(size_t)) + 0) = (uint8_t)2;
  *((uint8_t*)allocator.start + (1 * sizeof(size_t)) + 1) = 1;
  *((uint8_t*)allocator.start + (2 * sizeof(size_t)) + 1) = (uint8_t)3;
  *((uint8_t*)allocator.start + (2 * sizeof(size_t)) + 2) = 1;
  *((uint8_t*)allocator.start + (3 * sizeof(size_t)) + 2) = (uint8_t)4;
  *((uint8_t*)allocator.start + (3 * sizeof(size_t)) + 3) = 1;
  *((uint8_t*)allocator.start + (4 * sizeof(size_t)) + 3) = (uint8_t)5;

  allocator.print_records();

  std::free((void*)allocator.start);
  return 0;
}
