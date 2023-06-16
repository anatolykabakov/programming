# Introduction

Designing a system is a multi-step process that includes gathering requirements,
identifying entities and relationships, visualizing system architecture,
breaking down the system into smaller modules, creating class diagrams,
identifying functionalities and algorithms, and selecting technologies,
frameworks, and libraries.

# Gathering Requirements

Gathering the functional and non-functional requirements of a System is the
starting point in designing the system.

## Functional Requirements

Functional requirements are like a set of instructions for a system. They
describe what the system should do and how it should do it. They cover what the
system should accept as input, what it should produce as output, and how it
should interact with users and other methods. These requirements are essential
because they help ensure that the result is what everyone wants and needs.

The following are the functional requirements for a Library Management System:

- Ability to add and remove books from the library
- Ability to search for books in the library by title, author, or ISBN
- Ability to check out and return books
- Ability to display a list of all books in the library
- Ability to store and retrieve information about library patrons, including
  their name and ID number
- Ability to track which books are currently checked out and when they are due
  to be returned
- Ability to generate reports on library usage and checkouts

## Non-Functional Requirements

Non-functional requirements describe how a system should work, not just what it
should do. They include how fast it should run, how secure it should be, and how
easy it should be to use. They help make sure the system works well and meets
people's expectations.

The following are the non-functional requirements for a Library Management
System:

- User-friendly interface for easy navigation and use
- High performance and scalability to handle large amounts of data
- Data security and protection to ensure the privacy and confidentiality of
  library patrons and their information
- Compatibility with various operating systems and devices
- Ability to handle multiple users and concurrent access to the system
- Compliance with relevant laws and regulations regarding library management and
  data privacy
- Regular updates and maintenance to ensure the system remains functional and
  secure over time.

# Objects & Classes

Обьект -- некоторая сущность, которая содержит информацию (data/information) и
имеет поведение (behavior). Например, обьект машина содержит информацию о цвете,
количестве колес и тд. Поведение машины можно описать характеристикой drive stop
acceleration.

Object

- properties
- behavior

Car

- properties: 4 колеса, синий цвет
- behavior: drive, stop, acceleration

# Noun verb technique

Это техника для конвертации проблем реального мира в классы. Нужно составить
описание use case и выделить существительные noun и их поведения. Нужно выделить
поведения для существительных, обращать внимание на глаголы. Если классов
предметной области больше 1, то стоит составить схему взаимодействия.

Например:

Use case 1: Покупка товара на сайте

1. Пользователь заходит на сайт и выбирает слоты и классы, которые хочет купить
2. Добавляет карту
3. Совершает покупку
4. Получает подтверждение и чек

Классы предметной области Nouns: Пользователь, слоты, классы, карта Поведения
классов предметной области: выбирает, добавляет карту, совершает покупку

Упражнение: попробуйте взять любую задачу low level design и придумать use case,
подумать над use case, затем преобразовать вариант использования в классы
доменной области, выделив существительные и их поведения, взаимодействия
обьектов.

# Classes relationship

Классы должны взаимодействовать таким образом, чтобы выполнять вариант
использования use case. Существует два типа отнощение has is & is

## Composition & Aggregation

Composition -- Отношение принадлежности, когда класс зависит от другого. Клиент
имеет кредитную карту. Кредитная карта не может сама существовать без клиента.
Рисуется закрашенным ромбиком.

Aggregation -- Отношение принадлежности, когда класс не зависит от другого.
Напримре, корзина содержит продукты, но продукты могут существовать без корзины.
Рисуется ромбиком.

## Inheritance

Является типом. Напр, Видео урок и тестовый урок является производным типа урок.
