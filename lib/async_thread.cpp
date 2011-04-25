#include "async_file.h"

using std::list;

void * async_thread( void * ptr )
{
async_file * file = (async_file*)ptr;
std::list<block*>::iterator i;

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
		//Do Work
		}
	}

pthread_mutex_unlock( &file->lock );
pthread_exit( NULL );
}
