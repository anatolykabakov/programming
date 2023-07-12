# Desing URL Shortener service

## Background

Необходимо спроектировать сервис сокращения URL адресов, аналог tinyurl.
Пользователь вставляет длинную ссылку, нажимает кнопку и получает короткую
ссылку.

## Clarify requirements

### Functional requirements

- Конвертация длинной ссылки в короткую
- Нужно ли редиректить короткую ссылку на длинную?
- Какая длина ссылки? Длина ссылки должна быть 8 символов, состоять из цифр 0-9
  и букв a-z A-Z. 62^8 степени
- Сколько нужно хранить ссылку? Есть ли ограничения по ресурсам хранения?

### Non-Functional requirements

- Какой availability?
- Какой fault tolerance?
- Cколько операций чтения(чтение ссылки и редирект) и записи(создания ссылки)?
  -- отношение W:R 1:100
- Сколько ссылок создается каждый месяц? - 1 Billion? 32 M/day 400 W/sec
- Какой обьем хранилища? Одна ссылка 8 bytes. 1 Billion bytes = 8 GB для
  хранения коротких ссылок. Длинная ссылка примерно 1000 байт. Умножить на
  миллиард получим 1 TB.

## High-level design

### Components

- База данных для хранения URL. NoSQL
- Пользователь
- Сервер
- Сервис URL Generator
- Сервис удаления старых ссылок

### Relations

- База данных хранит ссылки, принимает запросы на чтение и запись от сервера
- Пользователь посылает запрос серверу на создание ссылки, принимает ответ
  сервера с короткой ссылкой. может переходить на котороткую ссылку, посылая
  запрос серверу на чтение, сервер редиректит короткую ссылку на длинную.
- Сервер редиректит короткую ссылку на длинную.
- Сервис URL Generator. Генерирует короткую ссылку.
- Cache хранит данные в памяти, ускоряет обработку запросов сервера.

## Design details

- Cache может быть LFU или LRU. Размер кэша 256 гигабайт.
- Как обрабатывать коллизии? Какой алгоритм хэширования?

### Alghorithm

### Data schema

### Bottlenecks

https://medium.com/@sandeep4.verma/system-design-scalable-url-shortener-service-like-tinyurl-106f30f23a82

https://www.geeksforgeeks.org/how-to-design-a-tiny-url-or-url-shortener/

https://www.docker.com/blog/how-to-build-and-deploy-a-django-based-url-shortener-app-from-scratch/

Реализация на go https://www.youtube.com/watch?v=rCJvW2xgnk0

Реализация python
https://www.youtube.com/watch?v=YI16KWyA3M0&list=LL&index=2&t=1389s
