#ifndef ASYNC_FILE_H
#define ASYNC_FILE_H

#include <pthread.h>

#include <list>
#include <iostream>

#include <sys/stat.h>

#include "async_block.h"
#include "async_stats.h"

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

	//control
	bool do_open( const char * filename, int flags, int mode );
	void do_close();

	//io
	size_t do_read( void * data, size_t offset, size_t length );
	size_t do_write( void * data, size_t offset, size_t length );
	size_t do_cut( void * data, size_t offset, size_t length );
	size_t do_insert( void * data, size_t offset, size_t length );
	size_t do_delete( size_t offset, size_t length );

	//syncro functions
	void do_flush( void ); //flush to filesystem
	void do_fsync( void ); //flush to filesystem to disk

	//Debugging stuff
	void print_backend( void );

	//when on, changes will be automatically written to disk
	//when off, changes won't be automatically written to disk
	void set_autoflush( bool on );

private:
	pthread_mutex_t   lock;
	pthread_t         thread;
	std::list<block*> objects;
	int               fd;
	bool              running;
	bool              autoflush;

	int flags;   //Underlying file descriptor flags
	struct stat fstats; //Underlying file stats, updated on write (not sure if by client or thread yet

	bool check_sanity();

	std::list<block*>::iterator find_block( size_t search_offset, size_t & block_offset );
	size_t count_preceding_bytes( std::list<block*>::const_iterator );
	void do_checks();

	//async_thread.c stuff
	std::list<block*>::iterator find_first_unsynced_block();
	void purge_dependencies( size_t offset, size_t length );

};


#endif //ASYNC_FILE_H
