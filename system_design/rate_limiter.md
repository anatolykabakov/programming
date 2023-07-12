# Desing Rate Limiter

## Background

Rate Limiter -- сервис, который ограничивает количество операций пользователя.
Например, количество загрузок видео на YouTube. YouTube это бизнес, который
платит деньги за хранение видеороликов, хранить очень много роликов невыгодно,
поэтому нужен сервис, который ограничивает количество загружаемых видео.

## Clarify requirements

Первым делом, нужно уточнить требования. Требования бывают функциональные и
нефункциональные.

### Functional requirements

Описываются высокоуровневые функционые требования. Обговариваются некоторые
важные детали.

Необходимо задать уточняющие вопросы:

1. Проектируем сервис для системы Backend API? - Да
2. Это должен быть отдельный микросервис или модуль, внутри микросервиса? -
   Отдельный микросервис, который имеет свой API
3. Лимитер должен отклонять запросы пользователя, если количество запросов
   больше порога. Если количество запросов пользователя меньше порога, то
   ограничений нет. Если количество запросов больше порога, то устанавливается
   запрет на использование API на N часов, ответ посылается 400 код ошибка.
4. Как определять пользователя, по IP, uid? Нужно ли записывать и хранить
   ограничения?

### Non-Functional requirements

Нефункциональные требования связаны с производительность, надежностью,
безопасностью и тд

- Latency должен быть минимальным, меньше 1 сек
- Throuput должен быть максимальным
- Storage сколько правил/ политик ограничений для разных сервисов
- Сколько пользователей пользуются приложением. Какого размера нужно хранилище
  для хранения информации IP пользователей и количества реквестов для алгоритма
  ограничения. IP 4 байта, информация о реквесте 128 байт. Если пользователей 1
  миллиард, то нужно 132 GB.
- Availability Какое отношение ошибок к корректным запросам хотим. 99.99? Как
  должно работать приложение, если сервис упал? - Если лимитер упал, то
  пользователи должны получать доступ к API или нет?

## High-level design

В этой части нужно представить полную высокоуровневую картину сервиса.
Сформулировать и нарисовать на схеме основные блоки.

### Components

1. Пользователь
2. Rate Limiter -- сервис, типа reverse-proxy
3. Rules -- политики ограничений (допустимо 20 загрузок в день). Хранятся в базе
   данных
4. Memory K/V Store для хранения хэшмапы IP пользователя и количества запросов
   для обработки ограничений.
5. Backend API

### Relations

1. Rate Limiter может читать политики из базы данных Rules
2. Пользователь может отправлять запрос Rate Limiter
3. Rate Limiter может читать, писать в Memory K/V Store для обработки
   ограничений для конкретных пользователей
4. Rate Limiter может отправлять запрос пользователя в Backend API

## Design details

Важные детали дизайна. Например, какой алгоритм для обработки ограничений

### Alghorithm

Алгоритм должен иметь пропускную способность 100 запросов в минуту. Сколько
памяти должен потреблять, какую временную сложность..

Известные алгоритмы

- Token Bucket
- Sliding window counter

### Data schema

Как располагаются данные в памяти, как эффективно использовать память. Какую
технологию использовать.

- Rules база данных содердит ключ id номер правила и поле описание правила
- Memory K/V Store хэшмапа, которая хранит ключ IP пользователя, значение
  количество запросов

### Bottlenecks

- Как машстабировать хранище?
- Как уменьшать Latency?
- Как увеличить Throuput?
- Как машстабировать горизонтально и вертикально?