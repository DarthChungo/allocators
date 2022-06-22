#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <limits>

void print_memory(void* start, size_t length) {
  uint8_t* current = (uint8_t*)start;

  for (size_t i = 0; i < length; i++, current++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)*current;
  }
}

class InlineHeaderAllocator {
 public:
  InlineHeaderAllocator(void* start, std::size_t size) : start(start), size(size) { std::memset(start, 0, size); }
  ~InlineHeaderAllocator() {}

  template <typename T>
  T* malloc(size_t s = sizeof(T)) {
    void* current = start;

    while (((uint8_t*)current - (uint8_t*)start) < (uint32_t)size) {
      if (*(size_t*)current == 0) {
        size_t* header   = (size_t*)current;
        current          = ((size_t*)current + 1);
        size_t available = 0;

        while (available != s && *(uint8_t*)current == 0) {
          current = ((uint8_t*)current + 1);
          available++;
        }

        if (available == s) {
          std::cout << "Found empty record: header=" << header << ", data=" << (header + 1) << ", size=" << s << "\n";
          *header = s;
          return (T*)(header + 1);
        }
      }

      current = (void*)((uint8_t*)current + *(size_t*)current + sizeof(size_t));
    };

    return nullptr;
  }

  template <typename T>
  void free(T* record) {
    assert(record && "Invalid address nullptr");
  }

  void print_records() {
    void* current = start;

    std::cout << "\nAllocator records (this=" << this << ", start=" << start << ", size=" << size << "):\n";

    while (*(size_t*)current != 0) {
      std::cout << "Record: header=" << current << ", data=" << ((size_t*)current + 1) << ", size=" << *(size_t*)current
                << ", data=0x";
      print_memory(((size_t*)current + 1), *(size_t*)current);
      std::cout << "\n";

      current = (void*)((uint8_t*)current + *(size_t*)current + sizeof(size_t));
    }

    std::cout << "Record: header ptr=" << current << " (empty)\n\n";
  }

 public:
  void*       start = nullptr;
  std::size_t size  = 0;
};

int main() {
  InlineHeaderAllocator allocator(std::malloc(1024), 1024);
  allocator.print_records();

  // manually test writing some records
  //*((uint8_t*)allocator.start + (0 * sizeof(size_t)) + 0) = 1;
  //*((uint8_t*)allocator.start + (1 * sizeof(size_t)) + 0) = (uint8_t)2;
  //*((uint8_t*)allocator.start + (1 * sizeof(size_t)) + 1) = 1;
  //*((uint8_t*)allocator.start + (2 * sizeof(size_t)) + 1) = (uint8_t)3;
  //*((uint8_t*)allocator.start + (2 * sizeof(size_t)) + 2) = 1;
  //*((uint8_t*)allocator.start + (3 * sizeof(size_t)) + 2) = (uint8_t)4;
  //*((uint8_t*)allocator.start + (3 * sizeof(size_t)) + 3) = 1;
  //*((uint8_t*)allocator.start + (4 * sizeof(size_t)) + 3) = (uint8_t)5;

  auto a = allocator.malloc<size_t>();
  auto b = allocator.malloc<size_t>();

  *a = std::numeric_limits<size_t>::max();
  *b = 1;

  allocator.print_records();
  std::free((void*)allocator.start);
  return 0;
}
