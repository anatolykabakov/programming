#include <iostream>
#include <vector>

struct Probe {
  Probe() = default;
  Probe(const Probe&) { ++copies; }
  Probe(Probe&&) /**noexcept**/ { ++moves; }

  static int copies;
  static int moves;
};

int Probe::copies = 0;
int Probe::moves = 0;

struct Frame {
  Frame(size_t size) : buf_(new int[size]), size_(size) {};
  Frame(Frame&& o) : buf_(o.buf_), size_(o.size_) { std::copy(o.buf_, o.buf_ + o.size_, buf_); }
  Frame& operator=(Frame&& o)
  {
    // if (this == &o) return *this;
    // Frame(std::move(o)).swap(*this);
    buf_ = o.buf_;
    size_ = o.size_;
    o.buf_ = nullptr;
    o.size_ = 0;
    return *this;
  }

  size_t size() { return size_; }

  int* buf_;
  size_t size_;
};

int main()
{
  std::vector<Probe> arr;
  arr.reserve(1);
  // arr.reserve(10);
  for (int i = 0; i < 5; ++i) {
    arr.push_back(Probe{});
    // arr.emplace_back();
  }

  std::cout << "copies " << Probe::copies << " moves " << Probe::moves << std::endl;

  Frame a(100);
  a = std::move(a);
  std::cout << a.size() << std::endl;
}
