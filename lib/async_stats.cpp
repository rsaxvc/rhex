#include "async_stats.h"
#include <iostream>

std::ostream& operator<<(std::ostream & stream, const async_file_stats & s )
{
stream<<"\tThere are "<<s.blocks<<" blocks"<<std::endl;
stream<<"\tThere are "<<s.unsynced_blocks<<" unsynced blocks"<<std::endl;
stream<<"\tThere are "<<s.fstats.st_size<<" total bytes in the file"<<std::endl;
stream<<"\tThere are "<<s.unsynced_size<<" bytes buffered in memory"<<std::endl;
}

