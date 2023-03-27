# Software Watermark

This repository explores algorithms for encoding/decoding/embedding/extracting arbitrary binary sequences from and to a function source code's CFG.

## How to Run

`make run` should run the main executable. From there you have some options:

```
1) encode string
2) encode number
3) encode number with reed solomon
4) generate graph from dijkstra code
5) run removal test
6) run removal test with improved decoding
7) run removal test with improved decoding and reed solomon
8) show report matrix
```

## Library

A dynamic library can be compiled with `make lib`. It will be available at `build/lib/libwatermark.so`.
