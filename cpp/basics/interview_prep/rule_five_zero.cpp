
#include <iostream>
#include <cstddef>

class Buffer {
public:
  Buffer(size_t size) : buf_(new int[size]), size_(size) { std::cout << "Buffer ctor" << std::endl; }

  Buffer(const Buffer& o) : buf_(new int[o.size_]), size_(o.size_)
  {
    std::copy(o.buf_, o.buf_ + o.size_, buf_);
    std::cout << "Buffer copy ctor" << std::endl;
  }

  Buffer& operator=(const Buffer& o)
  {
    Buffer(o).swap(*this);
    std::cout << "Buffer copy-assign ctor" << std::endl;
    return *this;
  }

  Buffer(Buffer&& o) noexcept : buf_(o.buf_), size_(o.size_)
  {
    o.buf_ = nullptr;
    o.size_ = 0;
    std::cout << "Buffer move ctor" << std::endl;
  }

  Buffer& operator=(Buffer&& o) noexcept
  {
    Buffer(std::move(o)).swap(*this);
    std::cout << "Buffer move-assogn ctor" << std::endl;
    return *this;
  }

  void swap(Buffer& o) noexcept
  {
    std::swap(o.buf_, buf_);
    std::swap(o.size_, size_);
    std::cout << "Buffer swap" << std::endl;
  }

  ~Buffer()
  {
    if (buf_) {
      delete[] buf_;
    }
    std::cout << "Buffer dtor" << std::endl;
  }

private:
  int* buf_;
  size_t size_;
};

class BufferZero {
public:
  BufferZero(size_t size) : buf_(size) {}

private:
  std::vector<int> buf_;
};

int main()
{
  Buffer a(100);
  Buffer b = a;
  b = a;

  Buffer c(std::move(a));
  Buffer d = std::move(b);

  std::vector<BufferZero> arr;
}
