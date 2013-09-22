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

if( numArgs < 2 )
	{
	cout<<"Usage: "<<args[0]<<" <filename> "<<endl;
	exit(1);
	}

if( !f.do_open( args[1], O_RDONLY, 0 ) )
	{
	cout<<"Error opening "<<args[1]<<endl;
	exit(2);
	}

f.print_backend();

cout
	<<"fstat:"<<f.get_status()
	<<"\nfsize:"<<f.get_size()
	<<endl;

f.print_backend();

for( int j = 0; j < f.get_size(); ++j )
	{
	char buf;
	if( f.do_read( &buf, j, sizeof( buf ) ) != 1 )
		{
		cout<<"Error reading file at offset:"<<j<<endl;
		exit(4);
		}
//	cout<<j<<"="<<hex<<(unsigned int)buf<<endl;
	}

f.print_backend();

cout<<f.get_stats()<<endl;
f.do_close();

}
return 0;
}
