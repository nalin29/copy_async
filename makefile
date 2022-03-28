all: copy_file.out copy_rec.out

copy_file.out: copy_async_file.c
	gcc -g copy_async_file.c -o copy_file.out

copy_rec.out: copy_async_rec.c copy.h datastructures.h
	gcc -g copy_async_rec.c -o copy_rec.out

clean:
	rm -rf *.out