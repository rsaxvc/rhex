#include "async_file.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>

using namespace std;
int main()
{{
async_file f;
int i;

f.do_open( "test.dat", O_RDWR | O_CREAT );

f.print_backend();

cout
	<<"fstat:"<<f.get_status()
	<<"\nfsize:"<<f.get_size()
	<<endl;

i=0;

void * kbuf;
kbuf = malloc( f.get_size() );
memset( kbuf, 0x00, f.get_size() );

f.print_backend();

f.do_write( kbuf, 1, 5 );
f.do_write( kbuf, 1, 5 );

int count;
for( int j = 0; j < 10000; ++j )
	{
	cout<<endl;
	cout<<"j="<<j<<endl;
	int where = rand() % f.get_size();
	int how_much = rand() % ( f.get_size() - where ) / 2;
	cout<<"Writing "<<how_much<<" bytes to "<<where<<endl;
	count = f.do_write(	kbuf, where, how_much );
	assert( count == how_much );
	}
f.print_backend();
free( kbuf );

f.print_backend();

//f.do_write( kbuf, 0, sizeof( kbuf ) );
f.do_close();

}
return 0;
}
