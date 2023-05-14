// https://www.programiz.com/dsa/heap-data-structure

#include <exception>
#include <iostream>
#include <vector>

class Heap {
public:
    Heap() { heap_.push_back(0); }

    void push(int value) {
        heap_.push_back(value);
        int i = heap_.size() - 1;

        while (i > 1 && heap_[i] < heap_[i / 2]) {
            int tmp = heap_[i];
            heap_[i] = heap_[i / 2];
            heap_[i / 2] = tmp;
            i /= 2;
        }
    }

    int pop() {
        if (heap_.size() == 1) {
            throw std::logic_error("heap empty!");
        }
        if (heap_.size() == 2) {
            int result = heap_[heap_.size() - 1];
            heap_.pop_back();
            return result;
        }

        int root = heap_[1];
        heap_[1] = heap_[heap_.size() - 1];
        heap_.pop_back();

        int i = 1;
        while (2 * i < heap_.size()) {
            if (2 * i + 1 < heap_.size() && heap_[2 * i + 1] < heap_[2 * i] &&
                heap_[i] > heap_[2 * i + 1]) {
                // Swap right child
                int tmp = heap_[i];
                heap_[i] = heap_[2 * i + 1];
                heap_[2 * i + 1] = tmp;
                i = 2 * i + 1;
            } else if (heap_[i] > heap_[2 * i]) {
                // Swap left child
                int tmp = heap_[i];
                heap_[i] = heap_[2 * i];
                heap_[2 * i] = tmp;
                i = 2 * i;
            } else {
                break;
            }
        }
        return root;
    }

    int root() { return heap_[1]; }

    void print() {
        for (const auto &el : heap_) {
            std::cout << el << std::endl;
        }
    }

private:
    std::vector<int> heap_;
};

int main() {
    // 14,19,16,21,26,19,68,65,30
    Heap h;
    h.push(14);
    h.push(19);
    h.push(16);
    h.push(21);
    h.push(26);
    h.push(19);
    h.push(68);
    h.push(30);
    h.print();
    h.push(17);
    h.print();
    std::cout << "pop " << h.pop() << std::endl;
    h.print();
}
