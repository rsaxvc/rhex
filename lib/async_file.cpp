#include "async_file.h"
#include "async_thread.h"

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

#include "minmax.h"

using std::cout;
using std::endl;

async_file::async_file()
{
pthread_mutexattr_t attr;
pthread_mutexattr_init( &attr );
pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
//create io thread
pthread_mutex_init( &lock, &attr );
pthread_mutexattr_destroy( &attr );
fd = -1;
running = false;
autoflush = false;

mmap_alignment = sysconf(_SC_PAGE_SIZE);
}

async_file::~async_file()
{
if( fd != -1 )
	{
	do_close();
	}
//sync with io thread
pthread_mutex_lock( &lock );
pthread_mutex_unlock( &lock );
pthread_mutex_destroy( &lock );
}

size_t async_file::do_read( void * data, size_t offset, size_t length )
{
std::list<block*>::const_iterator i;
size_t retn; //counts bytes copied
size_t block_offset; //records read position in file
size_t length_remaining; //bytes remaining to be transferred

retn = 0;
length_remaining = length;

pthread_mutex_lock( &lock );

//Find the first block in range
i = find_block( offset, block_offset );

if( i == objects.end() )
	{
	//Nothing found this far forward in the list
	return 0;
	}

for( ; i!= objects.end() && length_remaining; ++i )
	{
	//Find the spot and other math
	size_t position_in_block = offset - block_offset;
	size_t bytes_in_block = min( (*i)->size - position_in_block, length_remaining );

	//do the transfer
	memcpy( data, (uint8_t*)(*i)->ptr + position_in_block, bytes_in_block );

	//update bookkeeping
	retn += bytes_in_block;
	length_remaining -= bytes_in_block;
	block_offset += (*i)->size;
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
	assert( (objects.front())->type != BLOCK_TYPE_MEM );
	}

for( size_t i = objects.size(); i > 0; --i )
	{
	delete objects.back();
	objects.pop_back();
	}

close(fd);

fd = -1;
}

bool async_file::do_open( const char * filename, int file_flags, int mode )
{
fd = open( filename, file_flags, mode );

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
	sleep(1);
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

size_t async_file::do_write( void * _data, size_t offset, size_t length )
{
size_t temp_sz;
size_t out_count;
size_t bytes_for_this_block;
size_t block_offset;
std::list<block*>::iterator i;
uint8_t * data = (uint8_t*)_data;

bytes_for_this_block = 0;
out_count = 0;

while( length )
	{
	pthread_mutex_lock( &lock );
	print_backend();
	do_checks();

	i = find_block( offset, block_offset );

	//write starts past end of file, or write ends outside of file
	if( i == objects.end() )
		{
		//need to expand file here
		assert( false );
		}
	else
		{
		bytes_for_this_block = min( length, (*i)->size - ( offset - block_offset ) );
		std::cout<<"bftb:"<<bytes_for_this_block<<std::endl;
		std::cout<<"boff:"<<block_offset<<std::endl;
		std::cout<<"woff:"<<offset<<std::endl;
		assert( bytes_for_this_block != 0 );
		if( (*i)->type == BLOCK_TYPE_MEM )
			{
			memcpy( (uint8_t*)((*i)->ptr) + ( offset - block_offset ), data, bytes_for_this_block );
			}
		else if( (*i)->type == BLOCK_TYPE_MMAP )
			{
			size_t consumed_length;

			consumed_length = 0;

			//if possible, replace some of current block with mmap'd block
			temp_sz = (offset-block_offset)&~(mmap_alignment-1);
			if( temp_sz )
				{
				objects.insert( i, new block( fd, block_offset, temp_sz ) );
				consumed_length += temp_sz;
				}
			assert( ( consumed_length & (mmap_alignment-1) ) == 0 );

			//if needed, pad up to new block size
			temp_sz = mmap_alignment - (consumed_length&(mmap_alignment-1));
			if( temp_sz > 0 && temp_sz != mmap_alignment )
				{//fixup alignment requirements
				objects.insert( i, new block( (*i)->ptr + bytes_for_this_block, temp_sz, false ) );
				consumed_length += temp_sz;
				}
			assert( ( consumed_length & (mmap_alignment-1) ) == 0 );

			//install the new block
			objects.insert( i, new block( data, bytes_for_this_block, false ) );
			consumed_length += bytes_for_this_block;

			//if needed, pad out to the next mmap page size
			temp_sz = ( (*i)->size - consumed_length ) & (mmap_alignment - 1 );
			if( temp_sz > 0 )
				{//fixup alignment requirements
				objects.insert( i, new block( (*i)->ptr + consumed_length, temp_sz, false ) );
				consumed_length += temp_sz;
				}
			assert( ( consumed_length & (mmap_alignment-1) ) == 0 );

			//if possible, pad out the remainder with an mmap'd block
			temp_sz = ( (*i)->size - consumed_length ) & ~( mmap_alignment-1 );
			if( temp_sz )
				{
				objects.insert( i, new block( fd, block_offset + consumed_length, temp_sz ) );
				consumed_length += temp_sz;
				}
			assert( ( consumed_length & (mmap_alignment-1) ) == 0 );

			//if needed, pad out the remainder with a mem block
			temp_sz = ( (*i)->size - consumed_length );
			if( temp_sz )
				{
				objects.insert( i, new block( (*i)->ptr + consumed_length, temp_sz, false ) );
				}
			assert( ( consumed_length & (mmap_alignment-1) ) == 0 );

			assert( consumed_length == (*i)->size );
			delete( *i );
			objects.erase( i );
			}
		else
			{
			assert( false ); //bad block
			}
		length -= bytes_for_this_block;
		offset += bytes_for_this_block;
		data = (uint8_t*)data + bytes_for_this_block;
		out_count += bytes_for_this_block;
		}
	do_checks();
	pthread_mutex_unlock( &lock );
	}

return out_count;
}

void async_file::print_backend( void )
{
size_t filesize;

std::list<block*>::const_iterator i;
std::cout<<"Printing backend"<<std::endl;
std::cout<<"\tRelOffset/LocOffset/Type/size"<<std::endl;
filesize = 0;
pthread_mutex_lock( &lock );
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( (*i)->type == BLOCK_TYPE_MEM )
		{
		std::cout<<"\t"<<filesize<<"\t"<<filesize<<"\tmem:"<<(*i)->size<<" bytes"<<std::endl;
		filesize += (*i)->size;
		}
	else if( (*i)->type == BLOCK_TYPE_MMAP )
		{
		std::cout<<"\t"<<filesize<<"\t"<<(*i)->offset<<"\tmmap:"<<(*i)->size<<" bytes"<<std::endl;
		filesize += (*i)->size;
		}
	else
		{
		assert( false );
		}
	}
std::cout<<"\t"<<filesize<<"\tend of file"<<endl;
if( filesize != fstats.st_size )
	{
	cout<<"Filesize:"<<filesize<<endl;
	cout<<"stats.sz:"<<fstats.st_size<<endl;
	assert( filesize == fstats.st_size );
	}
pthread_mutex_unlock( &lock );
std::cout<<"Done printing backend"<<std::endl;
}

//Return an iterator to first block that holds the offset, and update the offset of that block
std::list<block*>::iterator async_file::find_block( size_t search_offset, size_t & block_offset )
{
std::list<block*>::iterator i;

block_offset = 0;

//Find the first block in range
for( i = objects.begin(); i!= objects.end() && block_offset + (*i)->size <= search_offset; ++i )
	{
	block_offset += (*i)->size;
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

size_t async_file::count_preceding_bytes(std::list<block*>::const_iterator search )
{
std::list<block*>::const_iterator i;
size_t retn;

retn = 0;

pthread_mutex_lock( &lock );
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( search != i )
		{
		retn += (*i)->size;
		}
	else
		{
		break;
		}
	}
pthread_mutex_unlock( &lock );
assert( i == search );
return retn;
}

void async_file::do_checks()
{
std::list<block*>::const_iterator i;
size_t cnt;

pthread_mutex_lock( &lock );
cnt = 0;
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( (*i)->type == BLOCK_TYPE_MMAP && (*i)->offset != cnt )
		{
		print_backend();
		std::cout<<"Bad offsets:"<<(*i)->offset<<" and "<<cnt<<std::endl;
		assert(false);
		}
	cnt += (*i)->size;
	}
if( fstats.st_size != cnt )
	{
	print_backend();
	assert(false);
	}
pthread_mutex_unlock( &lock );
}
