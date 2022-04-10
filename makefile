SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o .out .h

SRC_DIR := src

CC := gcc
CFLAGS := -Wall
LIBS := -lrt -luring
HEAD := $(SRC_DIR)/copy.h $(SRC_DIR)/list.h $(SRC_DIR)/logging.h

.PHONY: all clean release debug default

default: release

all: clean copy_aio_rec.out copy_uring_file.out copy_uring_inter_file.out copy_uring_rec.out

copy_async_file.out: $(SRC_DIR)/copy_async_file.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_async_file.c -o copy_async_file.out $(LIBS)

copy_file_inter.out: $(SRC_DIR)/copy_async_inter_file.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_async_inter_file.c -o copy_file_inter.out $(LIBS)

copy_aio_rec.out: $(SRC_DIR)/copy_async_rec.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_async_rec.c -o copy_aio_rec.out $(LIBS)

copy_uring_file.out: $(SRC_DIR)/copy_uring_file.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_uring_file.c -o copy_uring_file.out $(LIBS)

copy_uring_inter_file.out: $(SRC_DIR)/copy_uring_inter_file.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_uring_inter_file.c -o copy_uring_inter_file.out $(LIBS)

copy_uring_rec.out: $(SRC_DIR)/copy_uring_rec.c $(HEAD)
	$(CC) -I$(SRC_DIR) $(CFLAGS) $(SRC_DIR)/copy_uring_rec.c -o copy_uring_rec.out $(LIBS)		

clean:
	rm -rf *.out *.o

release: CFLAGS += -Ofast
release: copy_aio_rec.out copy_uring_rec.out

debug: CFLAGS += -g 
debug:all

