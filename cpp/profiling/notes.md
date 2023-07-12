# memory leak

```
g++ -g memory_leak.cpp -o memory_leak
valgrind --leak-check=full ./memory_leak
```

# callgrind

[tutorial](https://baptiste-wicht.com/posts/2011/09/profile-c-application-with-callgrind-kcachegrind.html)

```
g++ -g factorial.cpp -o factorial

valgrind --tool=callgrind ./factorial 10

```
