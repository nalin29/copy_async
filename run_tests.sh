#! /bin/bash
rm -rf test_src/
rm -rf test_dest/
python file_writer.py 32
printf "single op 32 kb," > results.txt
for i in {1..50} #change this to 5 or 10 for when running actual tests
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
printf "single op 1 mb," >> results.txt
for i in {1..50}
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
printf "single op 100 mb," >> results.txt
for i in {1..50}
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
python file_writer.py 32
printf "multi op 32 kb," >> results.txt
for i in {1..50} #change this to 5 or 10 for when running actual tests
do 
	./copy_aio_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1024
printf "multi op 1 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 102400
printf "multi op 100 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -i | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 32
printf "batched op 32 kb," >> results.txt
for i in {1..50} #change this to 5 or 10 for when running actual tests
do 
	./copy_aio_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1024
printf "batched op 1 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 102400
printf "batched op 100 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

rm -rf test_src/
python file_writer.py 1024
printf "Queue Depth 2," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 2 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 2 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 2 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 8," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 16," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 16 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 16 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 16 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 32," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 32 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 32 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 32 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 64," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f| tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 128," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 128 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 128 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 128 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Queue Depth 256," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "Fallocate off," >> results.txt
for i in {1..50}
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
for i in {1..50}
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

rm -rf test_src/
python file_writer.py 1024
printf "rb 32 kb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 2 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 2 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 32768 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 128 kb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 8 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 131072 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 512 kb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 16 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 16 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 524288 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 1 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 32 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 32 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 1048576 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 2 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f| tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 2097152 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 4 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 128 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 128 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 4194303 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "rb 8 mb," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -d -f -qd 256 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -d -f -qd 256 -rb 8388608 | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "O_DIRECT off," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -b -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_sync_rec.out test_src test_dest -b -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
	./copy_uring_rec.out test_src test_dest -b -f | tr -d "\n" >> results.txt
	rm -rf test_dest/
	printf "," >> results.txt
done
echo >> results.txt

printf "O_DIRECT on," >> results.txt
for i in {1..50}
do 
	./copy_aio_rec.out test_src test_dest -b -d -f | tr -d "\n" >> results.txt
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
