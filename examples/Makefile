CC=clang
SRC=$(wildcard *.c)
TARGET=$(SRC:.c=)

.PHONY: build run
$(TARGET): %: %.c
	$(CC) -g -O0 -o $@ $<

build: $(TARGET)
run: $(TARGET)
	$(foreach f,$^,valgrind --quiet ./$(f);)
