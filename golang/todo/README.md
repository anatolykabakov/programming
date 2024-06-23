# Todo list

Web application for tasks scheduling.

![UI](doc/ui.png "UI")

## Functional
1. Registration
2. Autorization
3. Create list of todo tasks
4. Create tasks in list

## REST API

The protocol for communications, based on HTTP, contains GET, POST, PUT and DELETE operations.

![API](doc/app.png "API")

### Endpoints
```
POST /auth/sign-in
POST /auth/sign-up

GET /lists
GET /lists/{id}
POST /lists
PUT /lists/{id}
DELETE /lists/{id}/items
GET /lists/{id}/items
POST /lists/{id}/items

PUT /items/{id}
GET /items/{id}
DELETE /items/{id}
```
## Data storage

Postgres database is used.

### Data schema

![DB](doc/db.png "DB")

## Code struct

1. cmd folder contains main.go
2. pkg folder contains all logic of application

Authorization test:
```
curl -X POST -d "{\"name\": \"Anatoly\",\"username\": \"Kabakov\", \"password\": \"qwerty\" }" http://localhost:8080/auth/sign-up
```

Authentication test:
```
curl -X POST -d "{\"username\": \"Kabakov\", \"password\": \"qwerty\" }" http://localhost:8080/auth/sign-in

responce
{"token":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3MTg1MTc3OTgsImlhdCI6MTcxODQ3NDU5OCwidXNlcl9pZCI6MX0.W010T4fN9g_8vKGBolU9mZBpQYJ5mZRhbnWwBHIHgFg"}
```

Create a list
```
request:
curl -X POST http://localhost:8080/api/lists/ -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3MTg2MDM4NDgsImlhdCI6MTcxODU2MDY0OCwidXNlcl9pZCI6MX0.USTnhiY5HDMBIO3xO_3bgDgPvlP_YYnsCt0BuF5fqLM" -d "{\"title\": \"second list\", \"description\": \"foo\"}"

responce:
{"id":1}
```

Get all lists
```
request:
curl -X GET http://localhost:8080/api/lists/ -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3MTg2MDM4NDgsImlhdCI6MTcxODU2MDY0OCwidXNlcl9pZCI6MX0.USTnhiY5HDMBIO3xO_3bgDgPvlP_YYnsCt0BuF5fqLM"

responce:
{"data":[{"id":1,"title":"first list","description":""},{"id":2,"title":"second list","description":"foo"}]}
```

Get list by id
```
request:
curl -X GET http://localhost:8080/api/lists/2 -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3MTg2MDM4NDgsImlhdCI6MTcxODU2MDY0OCwidXNlcl9pZCI6MX0.USTnhiY5HDMBIO3xO_3bgDgPvlP_YYnsCt0BuF5fqLM"

responce:
{"id":2,"title":"second list","description":"foo"}
```

db view

```
docker ps
docker exec -it id /bin/bash
psql -U postgres
\d
select * from users;
```
