#include "async_file.h"
#include <cassert>
#include <cstring>

#include <sys/mman.h>

block::block( void * buf, size_t byte_count, bool manage_buffer )
{
type = BLOCK_TYPE_MEM;
if( manage_buffer )
	{
	//caller asked us to manage the buffer
	ptr = buf;
	}
else
	{
	//caller asked us to not manage the buffer - make a copy instead
	ptr = malloc( byte_count );
	memcpy( ptr, buf, byte_count );
	}
size = byte_count;
offset = 0;
}

block::block( int fd, size_t temp_offset, size_t byte_count )
{
type = BLOCK_TYPE_MMAP;
ptr = mmap( 0, byte_count, PROT_READ, MAP_SHARED, fd, temp_offset);
size = byte_count;
offset = temp_offset;
assert( ptr != MAP_FAILED );
}

block::~block()
{
if( type == BLOCK_TYPE_MMAP )
	{
	munmap( ptr, size );
	size = 0;
	}
else if( type == BLOCK_TYPE_MEM )
	{
	std::free( ptr );
	ptr = NULL;
	}
else
	{
	assert( false );
	}
ptr = NULL;
size = 0;
}

