
#include <memory>
#include <iostream>
/**
 * @brief Reason Because private data members participate in class layout and private member functions participate in
 * overload resolution, changes to those implementation details require recompilation of all users of a class that uses
 * them. A non-polymorphic interface class holding a pointer to implementation (Pimpl) can isolate the users of a class
 * from changes in its implementation at the cost of an indirection.
 *
 */
class widget {
  class impl;
  std::unique_ptr<impl> pimpl_;

public:
  void draw();       // public API thar will be forwarded to the implementation
  widget(int data);  // defined in the implementation file
  ~widget();         // defined in the implementation file, where impl is a complete type
  widget(widget&&);  // defined in the implementation file
  widget(const widget&) = delete;
  widget& operator=(widget&&);  // defined in the implementation file
  widget& operator=(const widget&) = delete;
};

class widget::impl {
  int data_;  // private data
public:
  void draw(const widget& w) { std::cout << data_ << std::endl; }
  impl(int data) : data_{data} {}
};

void widget::draw() { pimpl_->draw(*this); }
widget::widget(int n) : pimpl_{std::make_unique<widget::impl>(n)} {}
widget::widget(widget&&) = default;
widget::~widget() = default;
widget& widget::operator=(widget&&) = default;

int main()
{
  widget w(42);
  w.draw();
  return 0;
}
