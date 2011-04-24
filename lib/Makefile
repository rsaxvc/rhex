all: read-tester write-tester

read-tester: read_test.o async_file.o
	g++ -o read-tester read_test.o async_file.o -lpthread

write-tester: write_test.o async_file.o
	g++ -o write-tester write_test.o async_file.o -lpthread

async_file.o: async_file.cpp async_file.h async_block.h async_stats.h
	g++ -o async_file.o async_file.cpp -g -O0 -c

read_test.o: async_file.cpp async_file.h async_block.h async_stats.h read_test.cpp

write_test.o: async_file.cpp async_file.h async_block.h async_stats.h write_test.cpp

clean:
	rm -f *.o read-tester write-tester
