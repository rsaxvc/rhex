#include "async_file.h"

#include <cassert>
#include <sys/mman.h>
#include <stdint.h>

#include "minmax.h"

#include <iostream>
using namespace std;

#define nap() usleep(10000) //up to 100fps latency
//	#define sleep(1); //extra latency for testing
//	#define pthread_yield();

#define MAX_CHUNK_SIZE 65536

using std::list;

list<block*>::iterator async_file::find_first_unsynced_block()
{
size_t offset;
list<block*>::iterator i;

offset = 0;
for( i = objects.begin(); i!= objects.end(); ++i )
	{
	if( (*i)->type == BLOCK_TYPE_MEM )
		{
		//Memory blocks are never synced to disk
		return i;
		}
	else if( (*i)->offset != offset )
		{
		//This block isn't located in a contiguous location
		return i;
		}

	offset += (*i)->size;
	}
}

//purge all dependencies on a region of the backing store by
//buffering to memory those blocks that would use the area
void async_file::purge_dependencies( size_t offset, size_t length )
{
size_t buf_start;
size_t buf_end;
size_t buf_length;
void * buf;
list<block*>::iterator i;

cout<<"Purging from "<<offset<<" "<<length<<"bytes"<<endl;

buf = malloc( length );
do
	{
	for( i = objects.begin(); i!= objects.end(); ++i )
		{
		if( (*i)->type != BLOCK_TYPE_MEM )//mem blocks are already buffered and have no dependency on physical disk
			{
			if( ( (*i)->offset + (*i)->size ) > offset && (*i)->offset < ( offset + length ) )
				{
				cout<<"Purging deps on block:offset:"<<(*i)->offset<<" and size:"<<(*i)->size<<endl;
				buf_start = (*i)->offset > offset ? (*i)->offset : offset;
				buf_end   = (*i)->offset + (*i)->size < offset + length ? ( *i)->offset + (*i)->size : offset + length;

				if( buf_start > buf_end )
					{
					cout<<"offset    = "<<offset      <<endl;
					cout<<"size      = "<<length      <<endl;
					cout<<"i->offset = "<<(*i)->offset<<endl;
					cout<<"i->size   = "<<(*i)->size  <<endl;
					cout<<"buf_start = "<<buf_start   <<endl;
					cout<<"buf_end   = "<<buf_end     <<endl;
					assert(false);
					}

				buf_length = buf_end - buf_start;

				do_read( buf, buf_start, buf_length );
				do_write( buf, buf_start, buf_length );

				//iterator is broken now, start the full pass again
				break;
				}
			}

		offset += (*i)->size;
		}
	}while( i != objects.end() );

free( buf );
}

void * async_thread( void * ptr )
{
void * write_ptr;
size_t write_size;
size_t written;
async_file * file = (async_file*)ptr;
std::list<block*>::iterator i;

pthread_mutex_lock( &file->lock );
while( file->running )
	{
	pthread_mutex_unlock( &file->lock );
	nap();
	cout<<"tick"<<endl;

	pthread_mutex_lock( &file->lock );
	if( file->autoflush && file->running )
		{
		i = file->find_first_unsynced_block();
		if( i != file->objects.end() )
			{
			//Do Work
			write_size = min( (*i)->size, MAX_CHUNK_SIZE );
			file->print_backend();
			cout<<"I->size:"<<(*i)->size<<endl;
			file->purge_dependencies( file->count_preceding_bytes(i), write_size );
			cout<<"I->size:"<<(*i)->size<<endl;
			i = file->find_first_unsynced_block();
			cout<<"I->size:"<<(*i)->size<<endl;
			if( write_size > (*i)->size )
				{
				cout<<"ws:"<<write_size<<endl<<"I->size"<<(*i)->size<<endl;
				file->print_backend();
				}
			assert( write_size <= (*i)->size );
			//i now points to the first unsynced block that is free of dependencies

			assert( i != file->objects.end() );

			write_ptr = mmap( 0, write_size, PROT_WRITE, MAP_SHARED, file->fd, (*i)->offset );
			written = file->do_read( write_ptr, (*i)->offset, write_size );
			munmap( write_ptr, write_size );
			assert( written == write_size );
			cout<<"count5:"<<file->lock.__data.__count<<endl;

			//Insert a new record for the freshly written bit
			file->objects.insert( i, new block( file->fd, (*i)->offset, write_size ) );
			if( (*i)->size > write_size )
				{
				//If the record we wrote from was bigger than the amount we wrote,
				//create a new memory buf object to hold the remainder.
				file->objects.insert( i, new block( (uint8_t*)((*i)->ptr) + write_size, (*i)->size - write_size, false ) );
				}
			delete( *i );
			cout<<"count6:"<<file->lock.__data.__count<<endl;
			}
		}
	}

pthread_mutex_unlock( &file->lock );
pthread_exit( NULL );
}
