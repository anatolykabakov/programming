#include <iostream>
#include <memory>
#include <vector>

class MyClass {
public:
  MyClass(int i) : i_(i) { vec_.reserve(i_); }

  MyClass(MyClass* c) : MyClass(c->i_ + 5) {}

  MyClass(const MyClass& c) : MyClass(c.i_ - 5) {}

  ~MyClass() { std::cout << "~MyClass: " << i_ << std::endl; }

private:
  int i_;  // 10
  std::vector<std::string> vec_;
};

int main()
{
  MyClass* c = new MyClass(10);
  try {
    auto s1 = std::shared_ptr<MyClass>(c);  // count 1
    auto s2 = std::shared_ptr<MyClass>(c);  // count 1
  }                                         // s1 удалит указатель.
  catch (...) {
  }
  return 0;
}
// My

int load_some_weight(...);
void load_image(...);

int n_rows = 100000;
int n_cols = 100;
float** image1 = new float*[n_rows];
float** image2 = new float*[n_rows];
float** image_out = new float*[n_rows];

for (int i = 0; i < n_rows; i++) {
  image1[i] = new float[n_cols];
  image2[i] = new float[n_cols];
  image_out[i] = new float[n_cols];
}

load_image(image1);
load_image(image2);

int a = load_some_weight();
int b = load_some_weight();
int c = load_some_weight();

// 2 x 2
// 0 = r * c + c
// 1 = r * c + c

for (int i = 0; i < n_cols; i++) {
  for (int j = 0; j < n_rows; j++) {
    image_out[i][j] = (image1[i][j] * a / c + image2[i][j] * b / c) * c;
    // img3 = (img1 * a / c + img2 * b / c) * c
    // img3 = img1 * a + img2 * b
  }
}
// 1. Создание трех двумерных массивов типа float
// 2. Загрузка данных в два изображения
// 3. Получаем веса a b c
// 4. Сумма двух изображений

// Задача: оптимизировать код.
// 1. in place модификация изображений
// 2. порядок обхода звумерного массива
// 3. Обратить внимание на формулую Конкретно, сократить количество операций деления и умножения.
