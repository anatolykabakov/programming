#include <iostream>

template <typename T>
class dynamic_array {
public:
  dynamic_array() : capacity_{2}, length_{0} { arr_ = new T[capacity_]; }

  ~dynamic_array() { delete[] arr_; }
  // Time O(1)
  const T& operator[](std::size_t index) const
  {
    if (index > length_) {
      throw "index out of bounds";
    }
    return arr_[index];
  }
  /**
   * @brief Puts element at the end of the array.
   * Time complexity O(1) if capacity > length.
   * O(N) If capacity is greater than the length of the array,
   * then the array will be resized, a new array will be created,
   * capacity will be multiplied by 2,
   * and all elements from the old array will be copied.
   *
   * @param value
   */
  void push_back(const T& value)
  {
    if (length_ == capacity_) {
      resize();
    }
    arr_[length_] = value;
    ++length_;
  }

  void resize()
  {
    capacity_ = 2 * capacity_;
    T* new_arr = new T[capacity_];

    for (uint i = 0; i < length_; ++i) {
      new_arr[i] = arr_[i];
    }

    std::swap(new_arr, arr_);

    delete[] new_arr;
    // Normally we would use smart pointers
  }

  std::size_t length() const { return length_; }

  void insert(std::size_t i, const T& value)
  {
    if (i < length_) {
      arr_[i] = value;
    }
    // Here we would throw an out of bounds exception
  }

  void pop_back()
  {
    if (length_ > 0) {
      --length_;
    }
  }

private:
  std::size_t length_;
  std::size_t capacity_;
  T* arr_;
};

int main()
{
  dynamic_array<int> arr;
  arr.push_back(1);
  arr.push_back(2);
  arr.push_back(3);
  for (std::size_t i = 0; i < arr.length(); ++i) {
    std::cout << arr[i] << std::endl;
  }
  arr.insert(0, 10);
  for (std::size_t i = 0; i < arr.length(); ++i) {
    std::cout << arr[i] << std::endl;
  }
  return 0;
}
