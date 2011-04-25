all: rhex bin2hex_test

rhex: rhex.o bin2hex.o
	g++ -o rhex rhex.o bin2hex.o -lncurses ../lib/async_file.o ../lib/async_thread.o ../lib/async_block.o -lpthread -lmagic

bin2hex_test: bin2hex_test.o bin2hex.o
	g++ -o bin2hex_test bin2hex_test.o bin2hex.o

bin2hex.o:
rhex.o:
bin2hex_test.o:
async_file.o: ../lib/async_file.o


clean:
	rm -f *.o rhex bin2hex_test a.out
