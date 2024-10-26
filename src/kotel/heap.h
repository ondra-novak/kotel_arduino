#include <utility>

template <typename T, typename Cmp>
void heap_push(T* heap, unsigned int size, Cmp cmp) {
    if (size < 2) return;
    unsigned int index = size - 1;
    while (index > 0) {
        unsigned int parent = (index - 1) / 2;
        if (cmp(heap[index], heap[parent])) {
            std::swap(heap[index], heap[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

template <typename T, typename Cmp>
void heap_pop(T* heap, unsigned int size, Cmp cmp) {
    if (size <= 1) return;

    std::swap(heap[0], heap[size - 1]);

    unsigned int index = 0;
    unsigned int last_index = size - 2;

    while (true) {
        unsigned int left = 2 * index + 1;
        unsigned int right = 2 * index + 2;
        unsigned int target = index;

        if (left <= last_index && cmp(heap[left], heap[target])) {
            target = left;
        }
        if (right <= last_index && cmp(heap[right], heap[target])) {
            target = right;
        }
        if (target == index) break;

        std::swap(heap[index], heap[target]);
        index = target;
    }
}
