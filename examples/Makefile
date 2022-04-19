CC=clang
TARGET=sort
SRC=sort.c

build:
	$(CC) -g $(SRC) -o $(TARGET)
run: build
	valgrind --quiet ./$(TARGET)
