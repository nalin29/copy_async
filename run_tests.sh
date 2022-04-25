#! /bin/bash
rm -rf test_src/
rm -rf test_dest/
python file_writer.py 32
printf "file size 32 kb," > results.txt
for i in {1..2} #change this to 5 or 10 for when running actual tests
do 
	./copy_aio_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1024
printf "file size 1 mb," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 102400
printf "file size 100 mb," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1048576
printf "file size 1 gb," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt


rm -rf test_src/
python file_writer.py 1024
printf "Queue Depth 8," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 64," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f| tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 256," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1048576
printf "Fallocate off," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Fallocate on," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f| tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Batching off," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -f -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -f -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -f -d | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Batching on," >> results.txt
for i in {1..2}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f| tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt
