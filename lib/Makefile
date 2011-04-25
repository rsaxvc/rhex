all: read-tester write-tester

read-tester: read_test.o async_file.o async_thread.o async_block.o async_stats.o
	g++ -o read-tester read_test.o async_file.o async_thread.o async_block.o async_stats.o -lpthread

write-tester: write_test.o async_file.o async_thread.o async_block.o async_stats.o
	g++ -o write-tester write_test.o async_file.o async_thread.o async_block.o async_stats.o -lpthread

async_file.o: async_file.cpp async_file.h async_block.h async_stats.h
	g++ -o async_file.o async_file.cpp -g -O0 -c

async_stats.o: async_stats.cpp async_stats.h async_block.h async_stats.h
	g++ -o async_stats.o async_stats.cpp -g -O0 -c

async_thread.o: async_thread.cpp async_thread.h async_block.h async_stats.h
	g++ -o async_thread.o async_thread.cpp -g -O0 -c

async_block.o: async_block.cpp async_block.h async_block.h async_stats.h
	g++ -o async_block.o async_block.cpp -g -O0 -c

read_test.o: async_file.h async_block.h async_stats.h read_test.cpp

write_test.o: async_file.h async_block.h async_stats.h write_test.cpp

async_file.h: async_thread.h async_block.h async_stats.h
	touch async_file.h

clean:
	rm -f *.o read-tester write-tester
