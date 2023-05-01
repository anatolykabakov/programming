#include <iostream>

template <typename Implementation>
struct ParserInterface {
  int getData() { return impl()->getDataImpl(); }

  int getID() { return impl()->getIDImpl(); }

private:
  Implementation* impl() { return static_cast<Implementation*>(this); }
};

struct ParserJsonImpl : public ParserInterface<ParserJsonImpl> {
  friend class ParserInterface<ParserJsonImpl>;

private:
  int getDataImpl() {
    std::cout << "ParserJsonImpl::getData()\n";
    return 0;
  }

  int getIDImpl() {
    std::cout << "ParserJsonImpl::getID()\n";
    return 0;
  }
};

struct ParserXmlImpl : public ParserInterface<ParserXmlImpl> {
  int getDataImpl() {
    std::cout << "ParserXmlImpl::getData()\n";
    return 0;
  }

  int getIDImpl() {
    std::cout << "ParserXmlImpl::getID()\n";
    return 0;
  }
};

template <typename Impl>
std::pair<int, int> parseFile(ParserInterface<Impl> parser) {
  auto result = std::make_pair(parser.getData(), parser.getID());
  return result;
}

int main() {
  ParserJsonImpl jsonParser;
  parseFile(jsonParser);

  ParserXmlImpl xmlParser;
  parseFile(xmlParser);

  return 0;
}
