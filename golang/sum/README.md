
'''
go run main.go

docker build -t calculator:test -f Dockerfile .
docker run -it --rm -p 8080:8080 docker.io/library/calculator:test

curl -X POST -d "{\"A\": 1, \"B\": 2, \"Operation\": \"+\"}" http://localhost:8080/calculator
curl -X POST -d "{\"Depth\": 1}" http://localhost:8080/history
'''
