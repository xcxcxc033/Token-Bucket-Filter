warmup2: bucket_filter.o my402list.o
	gcc -o bucket_filter -g bucket_filter.o my402list.o -lpthread -lm

warmup2.o: bucket_filter.c my402list.h
	gcc -g -c -Wall bucket_filter.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o bucket_filter
