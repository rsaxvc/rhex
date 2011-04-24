#include "async_file.h"
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/mman.h>

using std::cout;
using std::endl;

#ifndef min
	#define min( _lhs, _rhs ) ( ( _lhs > _rhs )? ( _rhs ) : ( _lhs ) )
#endif

#ifndef max
	#define max( _lhs, _rhs ) ( ( _lhs < _rhs )? ( _rhs ) : ( _lhs ) )
#endif

block::block( void * buf, size_t byte_count )
{
//cout<<"Generating new MEM block of "<<byte_count<<"bytes"<<endl;
type = BLOCK_TYPE_MEM;
ptr = malloc( byte_count );
size = byte_count;
memcpy( ptr, buf, byte_count );
offset = 0;
}

block::block( int fd, size_t temp_offset, size_t byte_count )
{
//cout<<"Generating new MMAP block of "<<byte_count<<"bytes"<<endl;
type = BLOCK_TYPE_MMAP;
ptr = mmap( 0, byte_count, PROT_READ, MAP_SHARED, fd, temp_offset);
size = byte_count;
offset = temp_offset;
if( ptr == NULL )
	{
	assert( false );
	}
}

block::~block()
{
if( type == BLOCK_TYPE_MMAP )
	{
	//cout<<"Destroying MMAP block of "<<size<<" bytes"<<endl;
	munmap( ptr, size );
	size = 0;
	}
else if( type == BLOCK_TYPE_MEM )
	{
	//cout<<"Destroying MEM  block of "<<size<<" bytes"<<endl;
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

void * async_thread( void * ptr )
{
async_file * file = (async_file*)ptr;

pthread_mutex_lock( &file->lock );
while( file->running )
	{
	pthread_mutex_unlock( &file->lock );
	usleep(10000); //up to 100fps latency
//	sleep(1); //extra latency for testing
//	pthread_yield();
//	std::cout<<"tick"<<std::endl;
	pthread_mutex_lock( &file->lock );

	if( file->autoflush )
		{
		}
	//Do Work
	}

pthread_mutex_unlock( &file->lock );
pthread_exit( NULL );
}

async_file::async_file()
{
pthread_mutexattr_t attr;
pthread_mutexattr_init( &attr );
pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
//create io thread
pthread_mutex_init( &lock, &attr );
pthread_mutexattr_destroy( &attr );
fd = -1;
}

async_file::~async_file()
{
//sync with io thread
pthread_mutex_lock( &lock );
pthread_mutex_unlock( &lock );
pthread_mutex_destroy( &lock );
}

size_t async_file::do_read( void * data, size_t offset, size_t length )
{
std::list<block*>::const_iterator i;
size_t retn; //counts bytes copied
size_t seek; //records read position in file
size_t length_remaining; //bytes remaining to be transferred

retn = 0;
length_remaining = length;
seek = offset; //init seek posn to offset

pthread_mutex_lock( &lock );

//Find the first block in range
i = find_block( seek );

if( i == objects.end() )
	{
	//Nothing found this far forward in the list
	return 0;
	}

for( ; i!= objects.end() && length_remaining; ++i )
	{
	//Find the spot and other math
	size_t position_in_block = offset - seek;
	size_t bytes_in_block = min( (*i)->size - position_in_block, length_remaining );

//	std::cout<<"Entered block"<<std::endl;
//	std::cout<<"\tposition_in_block"<<position_in_block<<std::endl;
//	std::cout<<"\tbytes_in_block:"<<bytes_in_block<<std::endl;
//	std::cout<<"\tlength_remains:"<<length_remaining<<std::endl;
//	std::cout<<"\tblock size"<<(*i)->size<<std::endl;

	//do the transfer
	memcpy( data, (uint8_t*)(*i)->ptr + position_in_block, bytes_in_block );

	//update bookkeeping
	retn += bytes_in_block;
	length_remaining -= bytes_in_block;
	seek += (*i)->size;
	}

pthread_mutex_unlock( &lock );
return retn;
}

size_t async_file::get_size( void )
{
std::list<block*>::const_iterator i;
size_t size;

size = 0;

pthread_mutex_lock( &lock );
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	size += (*i)->size;
	}
pthread_mutex_unlock( &lock );
return size;
}

void async_file::do_close( void )
{
//flush any pending ops
do_flush();

//Signal io thread
pthread_mutex_lock( &lock );
running = false;
pthread_mutex_unlock( &lock );

//Absorb the I/O thread
pthread_join( thread, NULL );

assert( objects.size() == 1 || objects.size() == 0 );
if( objects.size() == 1 )
	{
	assert( (objects.front())->type == BLOCK_TYPE_MEM );
	}
}

bool async_file::do_open( const char * filename, int file_flags )
{
fd = open( filename, file_flags );

if( fd != -1 )
	{
	flags = file_flags;
	if( 0 != fstat( fd, &fstats ) )
		{
		assert(false);
		perror("Could not get FSTATS while opening file");
		close( fd );
		return false;
		}

	pthread_mutex_lock( &lock );

	//Map the file
	block * ptr = new block(fd, 0, fstats.st_size );
	objects.push_back( ptr );

	//Start up I/O thread
	running = true;

	pthread_create( &thread, NULL, async_thread, this );

	pthread_mutex_unlock( &lock );
	return true;
	}
else
	{
	perror("error opening file");
	return false;
	}
}

void async_file::do_flush( void )
{
pthread_mutex_lock( &lock );
assert( running );
while( objects.size() > 1 )
	{
	pthread_mutex_unlock( &lock );
	pthread_yield();
	pthread_mutex_lock( &lock );
	}
pthread_mutex_unlock( &lock );
}

bool async_file::get_status( void )
{
bool retn;
pthread_mutex_lock( &lock );
retn = fd != -1;
pthread_mutex_unlock( &lock );
return retn;
}

size_t async_file::do_write( void * data, size_t offset, size_t length )
{
size_t out_count;
size_t bytes_for_this_block;
size_t seek = offset;
std::list<block*>::iterator i;

bytes_for_this_block = 0;

if( length <= 0 )
	{
	return 0;
	}

pthread_mutex_lock( &lock );

i = find_block( seek );

//write starts past end of file, or write ends outside of file
if( i == objects.end() )
	{
	//need to expand file here
	assert( false );
	}
else
	{
	bytes_for_this_block = min( length, (*i)->size - ( offset - seek ) );
	if( bytes_for_this_block == 0 )
		{
		assert(bytes_for_this_block != 0);
		}
	out_count = bytes_for_this_block;
	if( (*i)->type == BLOCK_TYPE_MEM )
		{
		memcpy( (uint8_t*)((*i)->ptr) + ( offset - seek ), data, bytes_for_this_block );
		seek += (*i)->size;
		length -= bytes_for_this_block;
		offset += bytes_for_this_block;
		data = (uint8_t*)data + bytes_for_this_block;
		if( length > 0 )
			{
			out_count += do_write( data, offset, length );
			}
		}
	else if( (*i)->type == BLOCK_TYPE_MMAP )
		{
		if( offset - seek && length < (*i)->size - ( offset - seek ) )
			{
            //Need to split block into mmap, mem, mmap, no need to continue
            objects.insert( i, new block( fd, (*i)->offset, offset - seek ) );
            objects.insert( i, new block( data, length ) );
            objects.insert( i, new block( fd, (*i)->offset + ( offset - seek ), (*i)->size - ( offset - seek ) - length ) );
			delete( *i );
			objects.erase( i );
			}
		else if( offset - seek ) //new object does not start on alignment of current block
			{
            //Need to split block into mmap, mem, may need to continue
            objects.insert( i, new block( fd, (*i)->offset, offset - seek ) );
            objects.insert( i, new block( data, bytes_for_this_block ) );
			delete( *i );
			objects.erase( i );

			//DoMeth
			seek += (*i)->size;
			length -= bytes_for_this_block;
			offset += bytes_for_this_block;
			data = (uint8_t*)data + bytes_for_this_block;
			if( length > 0 )
				{
				out_count += do_write( data, offset, length );
				}
			}
		else if( (*i)->size > length ) //new object is smaller than current block
			{
            //Need to split block into mem, mmap, no need to continue
            objects.insert( i, new block( data, bytes_for_this_block ) );
            objects.insert( i, new block( fd, (*i)->offset + ( offset - seek ), (*i)->size - ( offset - seek ) - length ) );
			delete( *i );
			objects.erase( i );
			}
		else
			{
			//Need to replace whole block, and possibly keep going
			objects.insert( i, new block( data, bytes_for_this_block ) );
			delete( *i );
			objects.erase( i );

			//DoMath
			seek += (*i)->size;
			length -= bytes_for_this_block;
			offset += bytes_for_this_block;
			data = (uint8_t*)data + bytes_for_this_block;
			if( length > 0 )
				{
				out_count += do_write( data, offset, length );
				}
			}
		}
	else
		{
		assert( false ); //bad block
		}
	}

pthread_mutex_unlock( &lock );
return out_count;
}

void async_file::print_backend( void )
{
size_t filesize;

std::list<block*>::const_iterator i;
std::cout<<"Printing backend"<<std::endl;

filesize = 0;
pthread_mutex_lock( &lock );
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( (*i)->type == BLOCK_TYPE_MEM )
		{
		filesize += (*i)->size;
		std::cout<<"\t"<<filesize<<"\tMemory object:"<<(*i)->size<<" bytes"<<std::endl;
		}
	else if( (*i)->type == BLOCK_TYPE_MMAP )
		{
		filesize += (*i)->size;
		std::cout<<"\t"<<filesize<<"\tmmap   object:"<<(*i)->size<<" bytes"<<std::endl;
		}
	else
		{
		assert( false );
		}
	}
std::cout<<"\t"<<filesize<<"\tend of file"<<endl;
if( filesize != fstats.st_size )
	{
	assert( filesize == fstats.st_size );
	}
pthread_mutex_unlock( &lock );
std::cout<<"Done printing backend"<<std::endl;
}

//Return an iterator to first block that holds the offset, and update the offset of that block
std::list<block*>::iterator async_file::find_block( size_t & offset )
{
std::list<block*>::iterator i;
size_t seek; //records read position in file

seek = 0;

//Find the first block in range
for( i = objects.begin(); i!= objects.end() && seek + (*i)->size <= offset; ++i )
	{
	seek += (*i)->size;
	}

if( i != objects.end() )
	{
	offset = seek;
	}

return i;
}

void async_file::set_autoflush( bool on )
{
pthread_mutex_lock( &lock );
autoflush = on;
pthread_mutex_unlock( &lock );
}

async_file_stats async_file::get_stats( void )
{
std::list<block*>::const_iterator i;
async_file_stats retn;

pthread_mutex_lock( &lock );
memset( (void*)&retn, 0x00, sizeof( retn ) );
retn.fstats = fstats;
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( (*i)->type == BLOCK_TYPE_MEM )
		{
		retn.unsynced_size += (*i)->size;
		retn.unsynced_blocks++;
		}
	retn.blocks++;
	}
pthread_mutex_unlock( &lock );
return retn;
}

