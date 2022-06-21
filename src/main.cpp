#include <iostream>
#include <cstddef>
#include <cstdlib>
#include <cassert>

struct AllocatorRecord {
  AllocatorRecord* next = nullptr;
  void*            ptr  = nullptr;
  bool             used = false;
  std::size_t      size = 0;
};

class LinkedListAllocator {
 public:
  LinkedListAllocator(void* base_ptr, std::size_t size) {
    max_records = size / (sizeof(AllocatorRecord) + 1);

    records       = (AllocatorRecord*)base_ptr;
    records->size = max_records;
    records->ptr  = (uint8_t*)base_ptr + (max_records * sizeof(AllocatorRecord));
  }

  ~LinkedListAllocator() {}

  template <typename T>
  T* malloc() {
    std::size_t size    = sizeof(T);
    auto        current = records;

    while (current) {
      if (!current->used && current->size >= size) {
        AllocatorRecord* next = current + size;

        if ((std::size_t)(next - records) != max_records) {
          next->size = current->size - size;
          next->ptr  = (uint8_t*)current->ptr + size;

          current->next = next;
        }

        current->used = true;
        current->size = size;

        return (T*)current->ptr;
      }

      current = current->next;
    }

    return nullptr;
  }

  template <typename T>
  void free(T* record) {
    assert(record && "Invalid address nullptr");
    AllocatorRecord* current = records;
    AllocatorRecord* prev    = nullptr;

    while (current) {
      if (current->ptr == record && current->used) {
        if (prev && !prev->used) {
          current = prev;
        }

        if (auto next = current->next; next && !next->used) {
          current->size += next->size;
          next->size = 0;
          next->ptr  = nullptr;

          current->next = next->next;
          next->next    = nullptr;
        }

        current->used = false;

        return;
      }

      prev    = current;
      current = current->next;
    }

    assert(false && "Trying to free an invalid record");
  }

  void print_records() {
    auto current = records;

    std::cout << "\nAllocator records (this=" << this << ", max_records=" << max_records << ", records=" << records
              << "):\n";

    while (current) {
      std::cout << "Record at address=" << current << ", ptr=" << current->ptr << ", size=" << current->size
                << ", used=" << current->used << ", next=" << current->next << "\n";

      current = current->next;
    }

    std::cout << "\n";
  }

 public:
  AllocatorRecord* records     = nullptr;
  std::size_t      max_records = 0;
};

int main() {
  LinkedListAllocator allocator(std::calloc(1024, 1), 1024);
  allocator.print_records();

  auto a = allocator.malloc<uint8_t>();
  auto b = allocator.malloc<uint8_t>();

  allocator.print_records();

  allocator.free(b);
  allocator.free(a);

  allocator.print_records();
  std::free((void*)allocator.records);
  return 0;
}
