all: copy_file.out copy_file_inter.out copy_rec.out copy_uring.out

copy_file.out: copy_async_file.c copy.h list.h
	gcc -g copy_async_file.c -o copy_file.out -lrt

copy_file_inter.out: copy_async_inter_file.c copy.h list.h
	gcc -g copy_async_inter_file.c -o copy_file_inter.out -lrt

copy_rec.out: copy_async_rec.c copy.h list.h
	gcc -g copy_async_rec.c -o copy_rec.out -lrt

copy_uring.o: copy_uring.c
	gcc -g -c copy_uring.c		

copy_uring.out: copy_uring.o
	gcc copy_uring.o -o copy_uring.out -luring 	


clean:
	rm -rf *.out *.o