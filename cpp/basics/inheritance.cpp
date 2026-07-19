#include <iostream>
#include <string>
#include <memory>
#include <vector>

class Person {
public:
  Person(const std::string& name) : name_(name) { std::cout << "Person ctor" << std::endl; }

  virtual void info() const { std::cout << "Name: " << name_ << std::endl; }

  const std::string& name() { return name_; }

  virtual ~Person() { std::cout << "Person dtoc" << std::endl; }

private:
  std::string name_;
};

class Employee final : public Person {
public:
  Employee(const std::string& name, const std::string& company) : Person(name), company_(company)
  {
    std::cout << "Employee ctor" << std::endl;
  }

  void info() const override
  {
    Person::info();
    std::cout << "Company: " << company_ << std::endl;
  }

  ~Employee() { std::cout << "Employee dtor" << std::endl; }

private:
  std::string company_;
};

class RuleOfFive {
public:
  explicit RuleOfFive(std::size_t n) : data_(new int[n]), size_(n) {}

  ~RuleOfFive() { delete[] data_; }
  // copy ctor
  RuleOfFive(const RuleOfFive& other)
  {
    size_ = other.size_;
    data_ = new int[size_];
    std::copy(other.data_, other.data_ + size_, data_);
  }
  // copy assign
  RuleOfFive& operator=(const RuleOfFive& other)
  {
    if (this == &other)
      return *this;
    delete[] data_;
    size_ = other.size_;
    data_ = new int[size_];
    std::copy(other.data_, other.data_ + size_, data_);
    return *this;
  }
  // move ctor
  RuleOfFive(RuleOfFive&& other) noexcept
  {
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
  }
  // move assign
  RuleOfFive& operator=(RuleOfFive&& other)
  {
    if (this == &other)
      return *this;
    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
    return *this;
  }

private:
  int* data_;
  std::size_t size_;
};

struct RuleOfFiveProd {
  std::vector<int> data;
};

int main()
{
  std::unique_ptr<Person> person = std::make_unique<Employee>("anatoly", "atom");
  person->info();

  // slicing
  Employee e("Top", "Google");
  Person p = e;
  p.info();
}
