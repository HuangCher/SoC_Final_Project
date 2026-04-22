CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=199309L

TARGET       = bloom_cli
TEST_TARGET  = test_bloom
SMOKE_TARGET = hw_smoke_test

SRCS       = main.c bloom_sw.c bloom_hw.c
TEST_SRCS  = test_main.c bloom_sw.c
SMOKE_SRCS = hw_smoke_test.c bloom_hw.c

all: $(TARGET) $(TEST_TARGET) $(SMOKE_TARGET)

$(TARGET): $(SRCS) bloom.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

$(TEST_TARGET): $(TEST_SRCS) bloom.h
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS)

$(SMOKE_TARGET): $(SMOKE_SRCS) bloom.h
	$(CC) $(CFLAGS) -o $(SMOKE_TARGET) $(SMOKE_SRCS)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(SMOKE_TARGET)

install: all
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(TEST_TARGET) $(SMOKE_TARGET) $(DESTDIR)/usr/bin/

.PHONY: all clean install
