#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

void print_memory(void* start, size_t length) {
  uint8_t* current = (uint8_t*)start;

  for (size_t i = 0; i < length; i++, current++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)*current;
  }

  std::cout << "\n";
}

class InlineHeaderAllocator {
 public:
  InlineHeaderAllocator(void* start, std::size_t size) : start(start), size(size) { std::memset(start, 0, size); }
  ~InlineHeaderAllocator() {}

  template <typename T>
  T* malloc(size_t s = sizeof(T)) {
    void* current = start;

    while (((uint8_t*)current - (uint8_t*)start) < (int32_t)size) {
      if (*(size_t*)current == 0) {
        size_t*  header    = (size_t*)current;
        uint8_t* data      = (uint8_t*)((size_t*)current + 1);
        uint8_t* head      = data;
        size_t   available = 0;

        while (true) {
          if (available == s) {
            *header = s;
            return (T*)data;

          } else if (*head != 0) {
            current = data + available;
            break;

          } else if (head - (uint8_t*)start >= (int32_t)size) {
            return nullptr;
          }

          available++;
          head++;
        }

      } else {
        current = (void*)((uint8_t*)current + *(size_t*)current + sizeof(size_t));
      }
    }

    return nullptr;
  }

  template <typename T>
  void free(T* record) {
    assert(record && "Invalid address nullptr");

    uint8_t* current = (uint8_t*)start;

    while ((current - (uint8_t*)start) < (int32_t)size) {
      if (current == (uint8_t*)record) {
        size_t* header = (size_t*)current - 1;
        std::memset(header, 0, *header + sizeof(size_t));
        return;
      }

      current++;
    }

    assert(false && "Passed invalid record");
  }

  void print_records() {
    std::cout << "\nAllocator records (this=" << this << ", start=" << start << ", size=" << size << "):\n";

    uint8_t* current = (uint8_t*)start;

    while ((current - (uint8_t*)start) <= (int32_t)size) {
      if (*current != 0) {
        std::cout << "Record: header=" << (void*)current << ", data=" << (void*)(current + sizeof(size_t))
                  << ", size=" << *(size_t*)current << "\n";
        current = (current + *(size_t*)current + sizeof(size_t));
        continue;
      }

      current++;
    }

    std::cout << "\n";
  }

 public:
  void*       start = nullptr;
  std::size_t size  = 0;
};

int main() {
  InlineHeaderAllocator allocator(std::malloc(1024), 1024);

  allocator.print_records();

  auto a = allocator.malloc<uint8_t>();
  auto b = allocator.malloc<uint8_t>();

  *a = 0xFF;
  *b = 0xFF;

  allocator.print_records();

  allocator.free(a);
  allocator.free(b);

  allocator.print_records();

  std::free((void*)allocator.start);
  return 0;
}
