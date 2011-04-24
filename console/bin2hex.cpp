#include "bin2hex.h"

#include <cstdlib>

bin2hex::bin2hex()
{

static const char nibble[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

typedef struct
	{
	char msn; //most significant nibble
	char lsn; //least significant nibble
	char nul; //string terminator
	}entry;

int i;

buf = (char*)malloc( 1<<8 * sizeof( entry ) );

for( i = 0; i < 256; ++i )
	{
	((entry*)buf)[i].msn = nibble[ i >> 4   ];
	((entry*)buf)[i].lsn = nibble[ i & 0x0F ];
	((entry*)buf)[i].nul = '\0';
	}
}

bin2hex::~bin2hex()
{
free( buf );
}

const char * bin2hex::operator()( unsigned char input )
{
typedef struct
	{
	char record[3];
	}entry;

return ((entry*)buf)[input].record;
}
