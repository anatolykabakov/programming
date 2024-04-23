docker build -t test_image:test -f Dockerfile .
docker run -it --rm -p 8080:8080 docker.io/library/test_image:test
curl -X POST -d "{\"A\": 1, \"B\": 2, \"Operation\": \"+\"}" http://localhost:8080/calculator
