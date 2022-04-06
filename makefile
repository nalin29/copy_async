all: copy_file.out copy_file_inter.out copy_rec.out

copy_file.out: copy_async_file.c copy.h list.h
	gcc -g copy_async_file.c -o copy_file.out -lrt

copy_file_inter.out: copy_async_inter_file.c copy.h list.h
	gcc -g copy_async_inter_file.c -o copy_file_inter.out -lrt

copy_rec.out: copy_async_rec.c copy.h list.h
	gcc -g copy_async_rec.c -o copy_rec.out -lrt

clean:
	rm -rf *.out