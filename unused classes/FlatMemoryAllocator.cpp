#include "FlatMemoryAllocator.h"

FlatMemoryAllocator::FlatMemoryAllocator(size_t maximumSize) : maximumSize(maximumSize), allocatedSize(0) {
    memory.reserve(maximumSize);
    initializeMemory();
}

FlatMemoryAllocator::~FlatMemoryAllocator() {
    memory.clear();
}

void* FlatMemoryAllocator::allocate(size_t size) {
    // Find the first available block that can accommodate the process
    for (size_t i = 0; i < maximumSize - size + 1; ++i) {
        if (allocationMap[i] == false && canAllocateAt(i, size)) {
            allocateAt(i, size);
            return &memory[i];
        }
    }
    // No available block found, return nullptr
    return nullptr;
}

void FlatMemoryAllocator::deallocate(void* ptr) {
    // Find the index of the memory block to deallocate
    size_t index = static_cast<char*>(ptr) - &memory[0];
    if (allocationMap[index]) {
        deallocateAt(index);
    }
}

std::string FlatMemoryAllocator::visualizeMemory() {
    return "";
}

void FlatMemoryAllocator::initializeMemory() {
    std::fill(memory.begin(), memory.end(), '.'); // '.' represents unallocated memory
    std::fill(allocationMap.begin(), allocationMap.end(), false);
}

bool FlatMemoryAllocator::canAllocateAt(size_t index, size_t size) const {
    // Check if the memory block is large enough
    return (index + size <= maximumSize);
}

void FlatMemoryAllocator::allocateAt(size_t index, size_t size) {
    // Mark the memory block as allocated
    std::fill(allocationMap.begin() + index, allocationMap.begin() + index + size, true);
    allocatedSize += size;
}

void FlatMemoryAllocator::deallocateAt(size_t index) {
    // Mark the memory block as deallocated
    allocationMap[index] = false;
}

void FlatMemoryAllocator::deallocateAt(size_t index) {
    // Mark the memory block as deallocated
    allocationMap[index] = false;
}



