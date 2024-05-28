package main

import (
	"fmt"
	"net/http"
	"time"
)

func ServeStatic(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case "GET":

		path := r.URL.Path

		fmt.Println(path)

		if path == "/"{
			path = "./static/index.html"
		}else{
			path = "." + path
		}

		http.ServeFile(w, r, path)
	default:
		fmt.Fprintf(w, "Request not supported")
	}
}

func Handle(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case "POST":
		r.ParseMultipartForm(0)
		message := r.FormValue("message")
		fmt.Println("post received: ", message)
		fmt.Fprintf(w, "Server logged message: %s \n", message + time.Now().Format(time.RFC3339))
	default:
		fmt.Fprintf(w, "Request not supported")
	}
}

func main() {
	http.HandleFunc("/", ServeStatic)
	http.HandleFunc("/handle", Handle)
	http.ListenAndServe(":5555", nil)
}
