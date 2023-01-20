# Watermark

Reed-Solomon code from: https://github.com/tierney/reed-solomon.

## How to Run

`make run` should run the main executable. From there you have some options:

```
1) encode string
2) encode number
3) generate graph from dijkstra code
4) run removal test
5) run removal test with improved decoding
6) run removal test with improved decoding and reed solomon
7) show report matrix
```

## Library

A dynamic library can be compiled with `make lib`. It will be available at `build/lib/libwatermark.so`.
