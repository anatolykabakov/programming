# Description

This is calculator service. Calculator support +-*/ operations. Calculator uses REST API to perform operations.

# API

## Endpoints

1. /calculator
2. /history

## Example calculator request

```
curl -X POST -d "{\"A\": 1, \"B\": 2, \"Operation\": \"+\"}" http://localhost:8080/calculator
```
## Example calculator responce

```
{"Value":3}
```

## Example history request

```
curl -X POST -d "{\"Depth\": 1}" http://localhost:8080/history
```
## Example calculator responce

```
["1 + 2 = 3"]
```

# Build & Run

```shell
go run main.go
```

```
docker build -t calculator:test -f Dockerfile .
docker run -it --rm -p 8080:8080 docker.io/library/calculator:test
```

# TODO list

- [x] Implement basic calculation operations +-*/
- [x] Add basic inmplementation REST API for calculator app.
- [x] Add mysql database to store operations log
- [x] Add Interfaces for handlers, services and storage
- [x] Add basic description: Short, Build, todos, referecnes
- [x] Add config for parameters
- [ ] Add tests for service layer
- [ ] Add docker compose for application

# References

1. [go-clean-arch](https://github.com/bxcodec/go-clean-arch)

2. [habr/739468](https://habr.com/ru/companies/otus/articles/739468/)

3. [build-your-first-rest-api-with-go-2gcj](https://dev.to/moficodes/build-your-first-rest-api-with-go-2gcj)
