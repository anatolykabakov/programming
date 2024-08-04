FROM golang:alpine

WORKDIR /go/src/

COPY . .

RUN go build -o main .

EXPOSE 8080

CMD ./main
