tester: main.o async_file.o
	g++ -o tester main.o async_file.o -lpthread

async_file.o: async_file.cpp async_file.h
	g++ -o async_file.o async_file.cpp -g -O0 -c

main.o: async_file.cpp async_file.h


clean:
	rm -f *.o tester

test: tester
	./tester
