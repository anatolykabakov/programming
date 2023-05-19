// 1. Реализовать класс домашнее животное
// 2.

1. класс Pet должен выполнять одну задачу.

    // file json_serializable.hpp
    class JsonSerializable {
public:
  virtual void from_json(const JsonNode& node) = 0;
  virtual JsonNode to_json() const = 0;
};

// file xml_serializable.hpp
class XmlSerializable {
public:
  virtual void from_xml(const XmlNode& node) = 0;
  virtual XmlNode to_xml() const = 0;
};

// file animal.hpp
class Pet : public JsonSerializable, public XmlSerializable {
public:
  enum class Type {
    DOG,
    PARROT,
    FISH,
  };

  Pet(const std::string& name, Type type, uint8_t age, bool is_speakable, size_t fin_count)
    : name_(name), type_(type), age_(age), is_speakable_(is_speakable), fin_count_(fin_count)
  {
  }

  void from_json(const JsonNode& node);
  JsonNode to_json() const
  {
    JsonNode json_node;
    json_node["name"] = name_;
    json_node["type"] = type_;
    json_node["age"] = age_;
    if (type_ == Type::PARROT) {
      json_node["is_speakable"] = is_speakable_;
    }
    if (type_ == Type::FISH) {
      json_node["fin_count"] = fin_count_;
    }

    return json_node;
  }

  void from_xml(const XmlNode& node);
  XmlNode to_xml() const
  {
    XmlNode xml_node;
    xml_node["name"] = name_;
    xml_node["type"] = type_;
    xml_node["age"] = age_;
    if (type_ == Type::PARROT) {
      xml_node["is_speakable"] = is_speakable_;
    }
    if (type_ == Type::FISH) {
      xml_node["fin_count"] = fin_count_;
    }

    return xml_node;
  }

  std::string full_name_info() const
  {
    std::ostringstream full_name;

    if (type_ == Type::DOG) {
      full_name << "dog - " << name << " - " << age << " years";
    } else if (type_ == Type::PARROT) {
      full_name << (is_speakable_ ? "speakable" : "") << " parrot - " << name << " - " << age << " years";
    } else {
      full_name << "fish with " << fin_count_ << " fins - " << name << " - " << age << " years";
    }

    return full_name.str();
  }

  void sit_down() const
  {
    if (type_ == Type::DOG) {
      std::cout << "sit down" << std::endl;
    }
  }

  void voice() const
  {
    if (type_ == Type::DOG) {
      std::cout << "Woof!" << std::endl;
    } else if (type_ == Type::PARROT && is_speakable_) {
      std::cout << (name_ + " is nice!") << std::endl;
    }
  }

private:
  std::string name_;
  Type type_;
  uint8_t age_;
  bool is_speakable_;
  size_t fin_count_;
};

// file main.cpp
void to_xml(const std::vector<std::shared_ptr<Pet>>& pets, XmlNode& root)
{
  for (const auto& pet : pets) {
    root.append(pet->to_xml());
  }
}

void to_json(const std::vector<std::shared_ptr<Pet>>& pets, JsonNode& root)
{
  for (const auto& pet : pets) {
    root.append(pet->to_json());
  }
}

int main()
{
  std::vector<std::shared_ptr<Pet>> pets;
  pets.push_back(std::make_shared<Pet>("Rex", Pet::Type::DOG, 7, false, 0));
  pets.push_back(std::make_shared<Pet>("Gosha", Pet::Type::PARROT, 3, true, 0));
  pets.push_back(std::make_shared<Pet>("Nemo", Pet::Type::FISH, 1, false, 5));

  auto xml_root = create_xml_root("pets.xml");
  to_xml(pets, xml_root);

  auto json_root = create_json_root("pets.json");
  to_json(pets, json_root);

  return 0;
};
