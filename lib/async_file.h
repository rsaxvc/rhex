#ifndef ASYNC_FILE_H
#define ASYNC_FILE_H

#include <pthread.h>

#include <list>
#include <iostream>

#include <sys/stat.h>

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
	//pthread_mutex_t       lock; //Lock this while modifying a chunk
public:
	block( int fd, size_t offset, size_t size );
	block( void * buf, size_t size );
	~block();
};

struct async_file_stats
	{
	struct stat fstats;
	size_t unsynced_size;
	size_t unsynced_blocks;
	size_t blocks;
	};

std::ostream& operator<<(std::ostream & stream, const async_file_stats & stats );

class async_file
{
	friend void * async_thread( void * );
public:
	//Constructs and Destructs
	async_file();
	~async_file();

	//Useful things
	bool get_status();
	async_file_stats get_stats();
	size_t get_unsynced_byte_count();
	size_t get_unsynced_chunk_count();
	size_t get_size();

	bool do_open( const char * filename, int flags );
	void do_close();

	size_t do_read( void * data, size_t offset, size_t length );
	size_t do_write( void * data, size_t offset, size_t length );
	size_t do_cut( void * data, size_t offset, size_t length );
	size_t do_insert( void * data, size_t offset, size_t length );
	size_t do_delete( size_t offset, size_t length );

	const  char * analyse();

	//syncro functions
	void do_flush( void ); //flush to filesystem
	void do_fsync( void ); //flush to filesystem to disk

	//Debugging stuff
	void print_backend( void );

	void set_autoflush( bool on );

private:
	pthread_mutex_t 	lock;
	pthread_t		thread;
	std::list<block*> objects;
	int			fd;
	bool running;
	bool autoflush;

	int flags;   //Underlying file descriptor flags
	struct stat fstats; //Underlying file stats, updated on write (not sure if by client or thread yet

	std::list<block*>::iterator find_block( size_t & offset );
	bool check_sanity();
};

#endif //ASYNC_FILE_H
