#ifndef ASYNC_STATS_H
#define ASYNC_STATS_H

#include <iostream>

struct async_file_stats
	{
	struct stat fstats;
	size_t unsynced_size;
	size_t unsynced_blocks;
	size_t blocks;
	};

std::ostream& operator<<(std::ostream & stream, const async_file_stats & stats );

#endif //ASYNC_STATS_H
