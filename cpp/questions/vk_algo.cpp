// # Scramble palindrome
// Нужно написать функцию, которая получает на вход строчку и проверяет является ли
// эта строка "перемешанным" палиндромом.

// f(aba) = true baa базовый случай. 2 a 1 b {"a": 2, "b": 1} count 1 true
// f(ABA) = true
// f(ABBA) = true {"A": 2, "B": 2} count = 0 true
// f(BBAA) = true {"A": 2, "B": 2} count = 0 true
// f(ABa) = false {"A": 1, "B": 1, "a": 1} count = 3 false
// f(AA) = true {"A": 2} count = 0 true
// f("") = true count = 0 true

// 1. Создать частотную карту ключ символ, значение количество в строке
// 2. Постротаем частотность символов в строке
// {"A": 2, "B": 2} для случая ABBA BBAA
// {"a": 2, "B": 1} для случая aba

// 3. Количество нечетных частот меньше 2

bool is_shuffled_palindlrome(const std::string& data)
{                                       // string_view
  std::unordered_map<char, bool> freq;  // std::vector<uint>
  for (const auto& c : data) {
    if (freq.find(c) == freq.end()) {
      freq.insert(std::make_pair(c, 1));
    } else {
      freq[c] += 1;
    }
  }

  std::size_t count = 0;
  for (const auto& [c, f] : freq) {
    if (f % 2 == 1) {
      count += 1;
    }
  }

  return count < 2;
}

// Задача 2

class Foo {
public:
  Foo() = delete;                 // Дефолтный конструктор
  Foo(int j) { i = new int[j]; }  // std::array - на стеке, применим std::vector
  virtual ~Foo() { delete[] i; }  // оператор []; виртуальный деструктор -virtual
private:
  int** i;  // добавить второй указатель unique_ptr
};

class Bar : public Foo {
public:
  Bar(int j) : Foo(j) { i = new char[j]; }  // явная инициализация
  ~Bar() { delete[] i; }                    // []
private:
  char* i;
};

template <typename T>
class myshared_ptr {
public:
  myshared_ptr() : ptr_{nullptr}, cnt_
  {
    new size_t(0) {}
  }
  myshared_ptr(T* ptr) : ptr_{ptr}, cnt_{new size_t(1)} {}

  ~myshared_() noexcept
  {
    if (ptr) {
      --cnt_;
      if (cnt == 0) {
        delete ptr_;
      }
    }
  }

private:
  T* ptr_;
  std::size_t* cnt_
}

void main()
{
  Foo* f = new Foo(100);
  myshared_ptr<Foo> p1(f);  // ptr_ == f
  myshared_ptr<Foo> p2(f);  // ptr_ == f

  std::weak_ptr<Foo> w(p1);
  w.lock();
  Foo* b = new Bar(200);
  *f = *b;
  delete f;
  delete b;
}
