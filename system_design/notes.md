# Background

## Computer architecture

### Disk

Основное хранилище в компьюере. Может сохранять информацию после выключения
компьютера. Обьем современных дисков TB(terabytes 10^2 bytes). Бывает разных
типов Hard disk drives(HDD), Solid-state drive(SSD). HDD механический, может
читать и писать, медленнее чем SSD. SSD стоит немного дороже. Скорость передачи
данных 1 MB порядка 1 миллисекунду 10^-3 sec.

### RAM

Решает задачу хранения информации. Быстрее, дороже чем SSD или HDD. Обьем в
диапазоне от 1-128 GB. Скорость передачи данных 1 MB порядка 1 микросекунды
10^-6 sec. Энергозависимая память, данные теряются после выключения компьютера.

Хранит данные приложений в памяти, включая переменные и выделенную память.

### CACHE

Память для хранения инструкций и данных. Быстрее, чем RAM и SSD. Может хранить
от нескольких килобайт KB 10^3 bytes до десятков MB 10^6 bytes.

### CPU

Для редактирования файла, расположенного на диске (внешняя память), программа
обработки загружает его в ОЗУ (внутренняя память), а конкретные символы, с
которыми в данные доли секунды работает процессор, «поднимаются» по иерархии
выше — в регистры процессора.

Написанный код компилируется в бинарный код, загружается в оперативную память.
Процессор читает и исполняет инструкции, которые включают обработку данных,
находящихся в оперативной памяти или диске. Процессор производит операции
сложения, вычитания, умножения.

Полезные ссылки:
https://xn----7sbbfb7a7aej.xn--p1ai/informatika_10_34_pol/informatika_materialy_zanytii_10_34_pol_13_13.html

## Application high-level architecture

Можно выделить основные компоненты.

### Developer

Разработчик пишет код.

### User

Пользователь использует приложение через интерфейс, например, веб браузер.

### Code Build & CI/CD & Git

Код версионируется, хранится в специальном хранилище, проходит автоматические
проверки, компилируется и поставляется в среду исполнения автоматически.

### Server

Это компьютер, где исполняется скомпилированный код приложения. Сервер может
принимать и отвечать на запросы пользователей. Если пользователей много, то
ресурсов сервера может не хватить, тогда требуется мастабирование сервера.
Вертикальным масштабированием называют увеличение ресурсов сервера, например,
больше оперативной памяти или мощнее процессор. Горизонтальным масштабированием
называют создание копии сервера, возможно, в другой физической локации.

### Storage

Обьем обрабатываемой приложением информации может превышать физические ресурсы
сервера, тогда инфомацию можно хранить в разных физических местах и передавать
по сети. Эффективным образом можно хранить информацию в специальных базах
данных.

### Load balancer

Если пользователей и серверов много, можно распределять нагрузку (запросы
пользователей) между серверами.

### Logging

Позволяет записывать действия приложения и отслеживать что случилось с
приложением в случае неполадки.

### Metrics

Метрики позволяют понять насколько хорошо работает приложение. Сколько запросов
пользователей успешно обработано.

### Alerts

Позволяет оповещать разработчиков в случае, если произошла неполадка. Например,
сервер стал обрабатывать 95 процентов запросов пользователей, метрики
зафиксировали изменений, разработчикам высылается оповещение о неисправности
приложения.

## Design Requirements

Проектирование системы можно свести к трем основным пунктам

1. Moving Data -- Перемещение данных между хранилищами: Базой данных, диском,
   оперативной памятью, процессором. Перемещение данных между разными серверами
   и пользоваталями, возможно, в разных географических локациях.
2. Storing Data -- Хранение данных. Необходимо ответить на вопросы какую задачу
   решаем и как хранить данные эффективно. базы данных? файловая система? Blob
   хранилище? Распределенное хранилище? Ответ зависит от конкретного use case
   приложения.
3. Transforming Data -- Преобразование данных. Например, преобразование лог
   файла о работе приложения в проценты успешных и неудачных запросов
   пользователя.

### Availability

Процент времени, когда приложение работает исправно c понедельника по пятницу с
9 утро до 5 вечера. Иногда требуется 24 часа 7 дней в неделе.

Availability = uptime / (uptime + downtime) %

99% = из 365 дней система не работала 3.65 дней. 99.9 downtime 0.1%

Availability измеряется в терминах 9s.

Хорошим показателем для большой компании является 99.999% 5 min downtime из 365
дней работы.

### Metrics

The measure of availability is used to define SLOs (service level objectives)
and SLAs (service level agreements).

SLA refers to an agreement a company makes with their clients or users to
provide a certain metric of uptime, responsiveness, and responsibilities. SLO
refers to an objective your team must hit to meet the SLA requirements. For
example, AWS's monthly SLA is 99.99% and if not met, they refund a percentage of
service credit.

### Reliability, Fault Tolerance, and Redundancy

Reliability способность приложения выполнять свою фукцию без падений или ошибок
определенное количество времени. Это вероятность, что сервер будет работать
корректно.

Fault Tolerance -- Способность приложения детектировать ошибки,
восстанавливаться к рабочему состоянию, переключаться на другой сервер если
доступен. Как легко сервер упадет или совершит ошибку, если будет DDoS атака.

Fault tolerance in system design is the ability of a system to continue
functioning normally, even in the presence of hardware or software failures.

Redundancy -- резервный потенциал. Приложение имеет резервный сервер, который
при нормальной работе не используется, но если основной сервер упал или ошибся,
то включается резервный сервер и выполняет задачу основного. Fault Tolerance
может быть только при наличии Redundancy.

Redundancy in system design is the duplication of critical components or
resources within a system to ensure the system's continued availability in the
event of a failure. It involves creating multiple copies of the same data,
software, hardware, or network components to provide a backup resource in the
case of an unexpected outage.

### Throughput

Пропускная способность -- количество данных, которое приложение обрабатывает за
период времени. Например, количество обработанных запросов пользователя в
секунду. Количество хранимых даных в год. Количество запросов приложения в базу
данных. Нагрузка на сеть. bytes/second.

Горизонтальное масштабирование увеличивает пропускную способность. Например,
добавили дополнительный сервер.

Вертикальное машстабирование позволяет увеличить пропускную способность на одном
сервере. Например, увеличили оперативную память.

### Latency

Время от начала запроса пользователя до получения ответа пользователем. Включает
в себя время отправки запроса по сети, обработку сервером, обратную отправку
приложением ответа пользователю.

# Networking

## Basics

Клиент посылает запрос или передаёт данные серверу. Сервер принимает запрос,
обрабатывает и отправляет ответ клиенту.

Каждый клиент и сервер имеет IP адрес, который состоит из 32 бит. IPv6 версии IP
128 бит. Бывает публичный и приватный IP. Приватный в локальной сети. Данные
передаются по интернет протоколу. Есть протокол http , это протокол уровня
приложения. Протокол Tcp транспортный. Данные передаются пакетами. Каждый пакет
содержит хидер, от кого IP кому IP, хидер Tcp (номер последовательности в
данных), сами данные.

Роутер имеет публичный IP и позволяет обрабатывать запросы из публичной сети
сервера к локальной сети.

Порты -- канал связи между клиентом и сервером. В http 80 портов, в https 443
порта.

### TCP & UDP

TCP -- протокол транспортного уровня. Гарантирует, что все пакеты будут
доставлены. Устанавливает соединение. Отслеживает количество отправленных и
принятых пакетов. Если пакет потерялся, то пересылает заново. Медленный, но
стабильный.

UDP -- протокол транспортного уровня. Не нужно устаналивать соединение и
отслеживать количество пакетов. Быстрее, по сравнению с TCP. Нет гарантии, что
все пакеты будут приняты. Используется в передаче видеопотока.

### Domain Name System (DNS)

Служба для конвертации понятного пользователю доменного имени напр google.com в
понятный компьютеру IP адрес сервера.

Client -google.com-> ISP(Internet Service Provider) --> ICANN --> Registry -IP->
Client -IP-> Server

### HTTP & RPC

RPC -- Remote procedure call. Например, пользователь заходит на сайт, загружает
фото и нажимает кнопку обработать. Запускается процедура обработки на сервере,
обрабатанная фотография отсылается пользоватлю по HTTP.

HTTP -- Hyper text transport protocol, протокол прикладного уровня. Совершает
запросы request и отправляет ответы responce. Пользователь отсылает request
информацию, сервер обрабатывает и отсылает результат responce. Имеет методы
GET(получить информацию от сервера, html страницу), PUT(обновить что-то),
POST(создать что-либо, аккаунт пользователя), DELETE. Возвращает статус код в
диапазоне 100 - 600 (200 -- успешно, 400 client error, 500 Server error)

HTTPs -- HTTP безопасный, SSL & TLS кодируют всю посылаемую информацию.

### Websockets

Например, хотим сделать онлайн чат. Нужно быстро обмениватсья сообщениями. HTTP
клиент посылает запрос и получает ответ. Это долго.

Вебсокет устанавливает соединение между клиентом и сервером. Клиент и сервер
могут посылать данные напрямую без запросов и ответов. Когда появляется новое
сообщение в чате, клиент получает информацию напрямую от сервера.

## Application programming interface (API)

это код, который позволяет двум приложениям обмениваться данными с сервера.

### Representation State Transfer REST

Построен поверх HTTP. Сервер не должен хранить информацию о состоянии
(проведенных операций) клиента. Каждый запрос от клиента должен содержать только
ту информацию, которая нужна для получения данных от сервера. Подходит для
горизонтального масштабирования. Формат данных JSON.

Пример REST API https://youtube.com/videos?offset=0,limit=10

URL:https://youtube.com/ OBJECT: videos PARAMETERS: offset=0,limit=10

### GraphQL

Относительно новый. Построен на HTTP. Использует только POST. Работа с данными
идет с помощью двух категорий запросов:на чтение (read) данных — в самом языке
их называют «запросы» (querys);на изменение (создание, обновление, удаление)
данных (для них применяют термин mutations).

В отличие от архитектуры REST, не требует наличия нескольких эндпойнтов для
каждого источника информации — все данные передаются через один шлюз уже
отфильтрованными схемой. То есть клиент (пользователь) получает именно ту
информацию, которую он запрашивал. В REST API для выполнения этой задачи
потребовалось бы создание или нового эндпойнта под каждый запрос, или одной
общей конечной точки с последующей фильтрацией данных уже на стороне самого
клиента доступными ему средствами.

https://blog.skillfactory.ru/glossary/graphql/

### gRPC

Построен на HTTP/2. Нативно не поддерживается браузером. Для использования нужен
прокси. Используется для server2server коммуникации. Быстрее, чем REST.
Используется для стриминга данных.

https://www.youtube.com/watch?v=bfdF4AJELDc

### API Design

Проектирование API можно сравнить с проектирование функций, сущностей. Важно
сформулировать контракт вход, выход приложения. Как пользователь будет
взаимодействовать с приложением. Если версия API меняется, то необходимо
поддерживать старую и новую версию.

Например, API Twitter

функция: создать твит. название функции createTweet(userId, content) вход:
создвать твит. REST запрос POST типа https://api.twitter.com/v1.0/tweet - POST.
В теле запроса есть параметры userId, content. Запрос представлен json. выход:
Tweet REST ответ обьект Tweet с полями userId, tweetId, content, createdAt,
likes

Для запроса конкретного твита можно создать API
https://api.twitter.com/v1.0/tweet/:Id - GET

Если хотим посмотреть несколько твитов
https://api.twitter.com/v1.0/users/:Id/tweets?limit=10&offset=0 - GET limit
сколько твитов запросить у юзера Id начиная с offset

## Caching

Важный концепт в дизайне. Кэш позволяет хранить наиболее используемую
информацию. Увеличивает пропускную способность за счет быстрого доступа к
используемой приложением информации.

Например, откроем браузер и зайдем на любой сайт. Браузер проверит нет ли в кэше
данных по этому запросу, если не обнаружит данных, то пошлет информацию на
сервер с целью загрузить данные с сервера и сохранить данные в кэш. Запрос к
серверу и скачивание данных может быть длительным процессом. В дальнейшем, при
запросе данных, будут загружаться данные из кэша, это быстрее.

Существуют разные уровни кэша:

1. Регистры процессора
2. Кэш процессора L1 L2 L3 (Быстрая память, мало обьема)
3. Оперативная память (Энергозависимая память, может работать с процессором.)
4. Диск (Энергонезависимая память, но слишком медленно для работы с процессором)
5. Удаленное хранилище( Надежно, но чтобы пользоваться нужно скачать по сети
   данные на компьютер с приложением.)

Напримео, когда клиент посылает запрос createTweet серверу. Сервер принимает
запрос, передает приложению, приложение исполняется процессором, процессор ищет
данные в памяти с высокого уровня, если не находит, то обращается к памяти
низкого уровня. Это занимает время. В следущий раз, запрос обрабаотается быстрее
за счет обращения к памяти высокого уровня.

write-back стратегия кэширование -- периодически записывать данные из
энергозависимой памяти в энергонезависимую для предотвращения потери информации
при падении сервиса.

Eviction cache -- вытеснение старых данных из кэша в пользу новых данных.
Существуют политики Least Recently Used(LRU) и Least Frequently Used(LFU).

## Content Delivery Network (CDN)

Имеется один сервер и множество клиентов по всему миру. Клиенту ближе к серверу
географически понадобится больше времени на загрузку контента. Клиенту дальше от
сервера понадобится больше времени за получение контента. Решением является CDN
-- статический сервер с контентом, расположенный в разных частях мира, чтобы
обеспечить равномерную раздачу информации. Клиенты в разных частях мира получают
информацию за одинаковое время.

Увеличивает latency и увеличивает availability.

Существуют Push / Pull CDN.

Логика работы CDN похожа на кэширование:

1. Клиент запрашивает данные у ближайшего CDN сервера
2. Если данные есть, то отправляются клиенту, иначе запрашиваются у главного
   сервера.

# Proxies

## Proxies & Load Balancers

Прокси - сервер посредник между клиентов и сервером. Клиент хочет выполнить
запрос, он посылает запрос прокси серверу, а тот посылает на сервер и передает
обратно прокси и клиенту. Прокси сервер нужен для выполнения промежуточной
работы, например, сокрытие данных пользователя, обход или установка блокировок,
сжатие траффика и др.

Бывает Forward и Reverse proxy. Forward прокси описан выше. Примером Reverse
прокси является Load balancer.

Load balancer позволяет равномерно распределить запросы пользователей по
множеству горизонтально масштабированных сервером. Пример алгоритма балансировка
Round Robin. Если серверы имеют разную мощность, то применяют алгоритм Weighted
Round Robin.

Балансировщики бывают на разных уровнях ISO. На транспортном уровне (уровень 4)
балансируется нагрузка траффика, например Round Robin отправляет запросы
равномерно всем серверам, чтобы пользователь получал быстрый ответ. На уровне
приложения (7 уровень) балансировка может быть по функциям приложениям.
Например, если клиент отправил запрос на твит, то балансировщик отправляет на
сервер, который обрабатывает твиты. Для других функций могут быть свои сервера и
нагрузка может балансироваться по функциям, потому что разные функции могут
занимать разное количество ресурсов.

Из опенсорсных балансировщиков можно выделить nginx.

https://habr.com/ru/companies/first/articles/683870/

## Consistent caching

Это метод балансировки запросов к серверу. Используем хэш функцию, которая
определяет как маппить IP юзера к ноде серверу. Если сервер, к которому ранее
был маппинг запросов клиента, упал, то запрос направится к ближайшему серверу.

Consistent Hashing is a distributed hashing scheme that operates independently
of the number of servers or objects in a distributed hash table by assigning
them a position on an abstract circle, or hash ring. This allows servers and
objects to scale without affecting the overall system.

https://habr.com/ru/companies/mygames/articles/669390/

https://bool.dev/blog/detail/algoritmy-kotorye-vy-dolzhny-znat-pered-sobesedovaniem

# Storage

Данные можно хранить в базах данных. Базы данных бывают реляционные(SQL) и
нереляционный(NOSQL)

## Standart Query Language (SQL)

Реляционные базы данных плохо горизонтально мастабируются !!!

Реляционный базы данных хранят данные на диске, потому что энергонезависимая
память. Базы данных хранят данные в структурах данных, предназначенных для
чтения и записи. Например, B+ Tree. Каждая нода дерева имеет 3 потомка и хранит
пару ключ значение. Значения в дереве хранятся в отсортированном порядке. Данные
хранятся в листьях дерева. Листья дерева соединены в список.

```
People
| PhoneNumber                 | Name     |
| -------------------- | ---------- |
| 123-456-7890               |       Alice     |
| 123-456-7891        | Bob |


Home
| PhoneNumber                 | Address     |
| -------------------- | ---------- |
| 123-456-7890               |       Moscow     |
| 123-456-7891        | NY |

```

Foreing key -- как правило, это общее поле для разных таблиц. Для таблиц People
& Home это PhoneNumber

Primary key -- идентифирует каждую строчку таблицы

Пример запроса вывести поля People.name, Home.address из таблиц People, Home, по
условию People.PhoneNumber == People.PhoneNumber

SELECT People.name, Home.address FROM People, Home WHERE People.PhoneNumber ==
People.PhoneNumber

### ACID

A - Atomicity C - Consistency I - Isolation D - Durability

### Transaction

База данных работает с транзакциями. Каждая транзакция начинается с начала,
включает операции чтения, записи, заканчивается коммитом.

## No relational (NOSQL)

Key-Value store: Redis, Memcached, etcd id -> value. Хорошо масштабируются.
Используют RAM. No schema

### Document Store

```
{
   "field1": {
      "onetype": [
         {"id": 1, "name": "Foo"},
         {"id": 2, "name": "Bar"},
      ]
   },
   "field2": {
      "onetype": [
         {"id": 1, "name": "Foo"},
         {"id": 2, "name": "Bar"},
      ]
   }
}
```

### Wide-Column

Write & no read & no update Mongo DB, Cassandra

### GraphDB

Используются, когда много соединений и зависимостей

### Eventional Consistency

Есть две базы данных DB1 & DB2. DB2 является копией и создана для
масштабирования. Как поддерживать консистентность данных при записи в DB1? DB1
становится leader базой, DB2 Follower. Лидер отвественнен за запись пришедших
данных в себя и всех подписчиков.

## Replication & Sharding

Replicationм -- процесс копирования базы данных. Например, есть схема Master --
Slave, когда есть база Master, в которую можно писать и читать. Master база
создает свою копию Slave, из которой можно читать. Выделяют синхронную и
асихронную репликацию. В синхронной запись добавляется в мастер базу, потом
сразу в slave базу. В асинхронной версии репликация может быть раз в час, в
день, в период. Существует схема Master -- Master. Можно читать и писать в обе
быза, но процесс репликации сложный.

Sharding -- разделение большой базы данных на мелкие. На практике сложный
процесс. Например, половина таблицы в базу 1, вторую потовину в базу 2. Как
определить где какая половина. Используют подход range based. Есть подход Hash
based. MySQL Postgres не имеют шардирования по дефолту, нужно реализовать
самостоятельно. NOSQL базы имеют шардирование.

## CAP Theorem

В теореме говорится, что если вы строите распределенную систему, то можете
удовлетворить только два из вышеупомянутых свойства, т.е. обязательно надо
пожертвовать одним из свойств.

Consistency (Согласованность). Как только мы успешно записали данные в наше
распределенное хранилище, любой клиент при запросе получит эти последние данные.
Avalability (Доступность). В любой момент клиент может получить данные из нашего
хранилища, или получить ответ об их отсутствии, если их никто еще не сохранял.
Partition Tolerance (Устойчивость к разделению системы). Потеря сообщений между
компонентами системы (возможно даже потеря всех сообщений) не влияет на
работоспособность системы. Здесь очень важный момент состоит в том, что если
какие-то компоненты выходят из строя, то это тоже подпадает под этот случай, так
как можно считать, что данные компоненты просто теряют связь со всей остальной
системой.

Хорошо написано про CAP теорему, простым языком.
https://habr.com/ru/articles/130577/ https://habr.com/ru/articles/136305/

PACEL -- Given P, choose A or C. Else, favor Latency or Consistency.

## Object Storage

Файловое хранищище.Файл имеет имя. Данные хранятся без иерархии. Пример AWS S3,
Google Disk. Blob -- бинарный файл, может быть тектовым файлом. Оптимально для
хранения больших файлов. Если хотим хранить изображения или видео или файлы,
нужно использовать обьектное хранилище, потому что много весят, неудобно хранить
и запросы на чтение и запись в базу будут долгими. Удобнно хранить, читать,
писать в файловых хранилищах. Чтение и запись происходит по HTTP.

# Big Data

## Message Queues

Главная идея очередей в том, что сервер не может обработать множество запросов
клиентов одновременно. Клиенты посылают запросы в очередь. Сервер берет запрос
из очереди и обрабатывает последовательно. Такую схему можно машстабировать.

## Pub/Sub pattern

Вариация техники очереди сообщений. Одно приложение публикует данные в
топик(шина событий), другое приложение подписывается на топик и читает сообщения
из топика и обрабатывает их. У топика может быть несколько подписчиков. Такая
система позволяет машстабировать систему. Пример ROS, Kafka, RabbitMQ

https://habr.com/ru/articles/270339/
https://ru.wikipedia.org/wiki/%D0%98%D0%B7%D0%B4%D0%B0%D1%82%D0%B5%D0%BB%D1%8C_%E2%80%94_%D0%BF%D0%BE%D0%B4%D0%BF%D0%B8%D1%81%D1%87%D0%B8%D0%BA

## MapReduce

MapReduce -- процесс раздельной обработки большого набора данных. Данные
разделяются на части и обрабатываются на разных машинах. Суть MapReduce состоит
в разделении информационного массива на части, параллельной обработки каждой
части на отдельном узле и финального объединения всех результатов.

https://www.bigdataschool.ru/wiki/mapreduce

### Batch & Streaming data processing

Batch data processing -- обработка данных, когда данные целиком доступны.
Например, нужно посчитать количество слов в книге.

Streaming data processing -- обработка данных в реальном времени часть данных.

# How to Approach

Нужно реализовать верхнеуровневый функционал или отдельный сервис-функцию.

Важно ограничить область решения, спросив интервьюера какую часть системы и
какую функциональность хочется спроектировать.

Важно собрать функциональные и нефункциональные требования. Функциональные
требования -- что должно делать функция. Например, нужно спроектировать функцию
лайк. Что должно происходить по нажатии на кнопку лайк. Что будет если нажать на
кнопку дважды? Должен ли сохраняться id пользователя, который нажал на кнопку?

Нефункциональные требования всегда про масштабирование и ограничения. Например,
реляционные базы данных плохо масштабируются, поэтому если хочется
масштабировать на 100 миллионов пользователей, то надо выбрать подходящую базу
данных.

Нужно спросить интервьюера какой масштаб системы. Сколько пользователей
одновременно могут пользоваться -- пропускная способность сервиса. Нужно ли
хранить данные? Если нужно, то какого размера хранилище потребуется. Какой
performance/latency у сервиса должен быть? Какие операции чаще, чтения или
записи? Спросить какой availability(отношение ошибок к количеству запросов)
99.99 или 99 хочется?

Пример:

Твиттером пользуется 100 миллионов человек ежедневно. Ежедневно, каждый человек
читает около 100 твитов. . Едеждневно происходит 10 миллиардов операций чтения.
Операций записи происходит намного сменьше, около 10 миллионов в день.

Throughput = 10B / 60 _ 60 _ 24 seconds = 86000 операций чтения в секунду. Нужно
взять запас 20 процентов для надежности. Нужно обеспечить 100000 операций
чтения.
