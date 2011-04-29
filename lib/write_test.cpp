#include "async_file.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>

using namespace std;
int main(int numArgs, char * args[] )
{{
async_file f;
int i;

srand(4);

if( numArgs < 2 )
	{
	cout<<"Usage: "<<args[0]<<" <filename> "<<endl;
	exit(1);
	}

if( !f.do_open( args[1], O_RDWR | O_CREAT ) )
	{
	cout<<"Error opening "<<args[1]<<endl;
	exit(2);
	}

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

int count;
if( f.get_size() > 0 )
	{
	for( int j = 0; j < 10000; ++j )
		{
		cout<<endl;
		cout<<"j="<<j<<endl;
		int where = rand() % f.get_size();
		int how_much = rand() % ( f.get_size() - where ) / 3;
		cout<<"Writing "<<how_much<<" bytes to "<<where<<endl;
		count = f.do_write(	kbuf, where, how_much );
		assert( count == how_much );

		//sleep(1);
		}
	}
else
	{
	cout<<"Not testing writes because file is empty"<<endl;
	}
f.print_backend();
free( kbuf );

f.print_backend();

cout<<f.get_stats()<<endl;
//f.do_write( kbuf, 0, sizeof( kbuf ) );
f.do_close();

}
return 0;
}
