#include <string>
#include <iostream>
#include <memory>

class BadWidget {
  std::string name;
  std::shared_ptr<BadWidget> otherWidget;

public:
  BadWidget(const std::string& n) : name(n) { std::cout << "BadWidget " << name << std::endl; }

  ~BadWidget() { std::cout << "~BadWidget " << name << std::endl; }

  void setOther(const std::shared_ptr<BadWidget>& x)
  {
    otherWidget = x;
    std::cout << name << " now points to " << x->name << std::endl;
  }
};

class GoodWidget {
  std::string name;
  std::weak_ptr<GoodWidget> otherWidget;

public:
  GoodWidget(const std::string& n) : name(n) { std::cout << "GoodWidget " << name << std::endl; }

  ~GoodWidget() { std::cout << "~GoodWidget " << name << std::endl; }

  void setOther(const std::shared_ptr<GoodWidget>& x)
  {
    otherWidget = x;
    std::cout << name << " now points to " << x->name << std::endl;
  }
};

int main()
{
  {  // В этом примере происходит утечка памяти
    std::cout << "====== Example 3" << std::endl;
    std::shared_ptr<BadWidget> w1(new BadWidget("3_First"));
    std::shared_ptr<BadWidget> w2(new BadWidget("3_Second"));
    w1->setOther(w2);
    w2->setOther(w1);
  }
  {  // А в этом примере использован weak_ptr и утечки памяти не происходит
    std::cout << "====== Example 3" << std::endl;
    std::shared_ptr<GoodWidget> w1(new GoodWidget("4_First"));
    std::shared_ptr<GoodWidget> w2(new GoodWidget("4_Second"));
    w1->setOther(w2);
    w2->setOther(w1);
  }
  return 0;
}
