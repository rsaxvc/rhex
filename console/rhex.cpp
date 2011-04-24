#include <ncurses.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <fcntl.h>
#include <magic.h>

#include "bin2hex.h"
#include "../lib/async_file.h"

using namespace std;

//print a char in bold
//addch( 'f'|A_BOLD );

void draw_screen( async_file & fd )
{
int x;
int y;
int ix;
int iy;
size_t size;
size_t max_size;

size_t iterator;

bin2hex b;

x = getmaxx( stdscr );
y = getmaxy( stdscr );

size = fd.get_size();
max_size = size;
if( max_size > x * y /2 )
	{
	max_size = x * y / 2;
	}

char * buf = (char*)malloc( max_size );

if( max_size != fd.do_read( buf, 0x00, max_size ) )
	{
	fd.do_flush();
	exit(3);
	}

if( y > 3 )
	{
	for( iy = 0; iy < y - 2; ++iy )
		{
		for( ix = 0; ix < x; ix+=3 )
			{
			move( iy, ix );
			if( iy * x + ix/2 <= max_size )
				{
				printw("%s", b(buf[iy*x + ix/2 ]) );
				}
			else
				{
				break;
				}
			}
		}
	}

free( buf );
}

void draw_byte_analysis( async_file & fd, size_t offset, int x, int y, int w )
{
magic_t m;

char * buf;
size_t size;

int ix;

if( y > 3 )
 	{
	m = magic_open( MAGIC_NONE );
	magic_load( m, NULL );

	size = 1024*1024;//assume 1 meg remaining
	buf = (char*)malloc( size );
	size = fd.do_read( buf, offset, size );
	for( ix = x; ix < x + w; ++ix )
		{
		move( y, ix );
		printw(" ");
		}
	move( y, x );
	printw( "%s", magic_buffer( m, buf, size ) );
	free( buf );

	magic_close( m );
	}
}

int main(int numArgs, const char * const args[] )
{
bool running = true;
int x=0;
int y=0;
int key;
size_t cursor=0; //position of cursor in file
size_t filesize=0;

if( numArgs != 2 )
	{
	cout<<"usage: "<<args[0]<<" filename"<<endl;
	exit(1);
	}

async_file fd;

if( false == fd.do_open( args[1], O_RDWR | O_CREAT ) )
	{
	cout<<"Couldn't open file"<<endl;
	exit(2);
	}

initscr();
cbreak();
keypad( stdscr, 1 );

while( running )
	{
	//move( y / 2, x/2 - strlen("Hello world!!! ")/2 );
	//printw("Hello World !!!");

	draw_screen( fd );
	draw_byte_analysis( fd, 0x00,   0, getmaxy( stdscr ) - 2, getmaxx( stdscr ) );
	draw_byte_analysis( fd, cursor, 0, getmaxy( stdscr ) - 1, getmaxx( stdscr ) );

	move( cursor / ( getmaxx( stdscr ) / 3 ), 3 * ( cursor % ( getmaxx( stdscr ) / 3 ) ) );
	refresh();

	key = getch();

	switch( key )
		{
		case KEY_UP:
			if( cursor > getmaxx( stdscr ) / 3 )
				{
				//move cursor one UI-line
				cursor-= getmaxx( stdscr ) / 3;
				}
			else
				{
				//heading past start of file
				cursor = 0;
				}
			break;

		case KEY_LEFT:
			if( cursor > 0 )
				{
				cursor--;
				}
			break;

		case KEY_RIGHT:
			if( cursor + 1 < fd.get_stats().fstats.st_size )
				{
				cursor++;
				}
			break;

		case KEY_DOWN:
			if( cursor + getmaxx( stdscr ) / 3 < fd.get_stats().fstats.st_size )
				{
				//move cursor one UI-line
				cursor+= getmaxx( stdscr ) / 3;
				}
			else
				{
				//heading past end of file
				cursor = fd.get_stats().fstats.st_size;
				if( cursor > 0 )
					{
					cursor--;
					}
				//else - if file is 0 bytes long, keep cursor at zero.
				}
			break;

		case KEY_EXIT:
			running = false;
			break;
		}
	}
endwin();

return 0;
}
