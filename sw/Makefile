CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=199309L

TARGET      = bloom_cli
TEST_TARGET = test_bloom

SRCS      = main.c bloom_sw.c bloom_hw.c
TEST_SRCS = test_main.c bloom_sw.c

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRCS) bloom.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

$(TEST_TARGET): $(TEST_SRCS) bloom.h
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS)

clean:
	rm -f $(TARGET) $(TEST_TARGET)

.PHONY: all clean
