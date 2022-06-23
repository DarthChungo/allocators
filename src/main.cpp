#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

void print_memory(void* start, size_t length) {
  size_t* current = (size_t*)start;

  std::cout << "\n";

  for (size_t i = 0; i < length; i++, current++) {
    std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex << i << ": 0x" << std::setfill('0')
              << std::setw(16) << std::hex << (int64_t)*current << "\n";
  }

  std::cout << "\n";
}

class InlineHeaderAllocator {
 public:
  InlineHeaderAllocator(void* start, std::size_t size) : start(start), size(size) { std::memset(start, 0, size); }
  ~InlineHeaderAllocator() {}

  template <typename T = void>
  T* malloc(size_t s = sizeof(T)) {
    size_t* current = (size_t*)start;
    size_t  length  = (s + sizeof(size_t) - 1) / sizeof(size_t);  // in multiples of sizeof(size_t)

    while ((uint8_t*)current - (uint8_t*)start < (int32_t)size) {
      if (*current == 0) {
        size_t* header    = current;
        size_t* data      = header + 1;
        size_t* head      = data;
        size_t  available = 0;

        while (true) {
          if (available == length) {
            *header = length;
            return (T*)data;

          } else if (*head != 0) {
            current = head + *head + 1;
            break;

          } else if ((uint8_t*)head - (uint8_t*)start >= (int32_t)size) {
            return nullptr;
          }

          available++;
          head++;
        }

      } else {
        current = current + *current + 1;
      }
    }

    return nullptr;
  }

  template <typename T>
  void free(T* record) {
    assert(record && "Invalid address nullptr");

    if (size_t* header = ((size_t*)record - 1); header != 0) {
      std::memset(header, 0, *header * sizeof(size_t) + sizeof(size_t));

    } else {
      assert(false && "Passed an invalid record");
    }
  }

  void print_records() {
    std::cout << "\nAllocator records (this=" << this << ", start=" << start << ", size=" << size << "):\n";

    size_t* current = (size_t*)start;

    while ((uint8_t*)current - (uint8_t*)start < (int64_t)size) {
      if (*current != 0) {
        std::cout << "Record: header=" << (void*)current << ", data=" << (void*)(current + 1)
                  << ", size=" << sizeof(size_t) * *current << "\n";
        current = (current + *current + 1);
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
  std::cout << "Before any allocations:\n";
  allocator.print_records();

  auto a = allocator.malloc(16);
  auto b = allocator.malloc(16);
  auto c = allocator.malloc(16);

  *(uint8_t*)a = *(uint8_t*)b = *(uint8_t*)c = 0xF;

  std::cout << "After three allocations:\n";
  allocator.print_records();

  allocator.free(b);

  std::cout << "After free:\n";
  allocator.print_records();

  auto d = allocator.malloc(8);
  auto e = allocator.malloc(32);

  *(uint8_t*)d = *(uint8_t*)e = 0xF;

  allocator.malloc(32);

  std::cout << "After more allocs:\n";
  allocator.print_records();

  std::free((void*)allocator.start);
  return 0;
}
