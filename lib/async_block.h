#ifndef ASYNC_BLOCK_H
#define ASYNC_BLOCK_H

#include <cstdlib>

class async_file;

enum block_type
	{
	BLOCK_TYPE_MMAP,
	BLOCK_TYPE_MEM,
	BLOCK_TYPE_COUNT
	};

class block
{
friend class async_file;
private:
	block_type type;
	void * ptr; //can be ptr to mem or ptr to mmap
	size_t size;

	size_t offset; //used to record where in a file this object points
public:
	block( int fd, size_t offset, size_t size );
	block( void * buf, size_t size, bool manage_buffer );
	~block();
};

#endif //ASYNC_BLOCK_H
