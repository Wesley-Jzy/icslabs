#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/*
 * name: 姜子悦
 * loginID: 515030910211
 */

/* Macro */
#define BITS 64
#define TRUE 1
#define FALSE 0
#define MISS 0
#define HIT 1
#define EVICTION 2

#define SET(addr) (((addr)<<t)>>(b+t))
#define TAG(addr) ((addr)>>(b+s))

/* Cache struct */
/* A line: valid|tag|block */
typedef struct line
{
	/* valid = 0,unused; else, used */
	int valid;
	unsigned long long tag;

	/* when used, count++ */
	unsigned long long count;
} line_t;

/* A set: lines[n] can visit nth line */
typedef line_t* set_t;

/* A cache: sets[n] can visit nth set */
typedef struct cache
{
	int s;
	int E;
	int b;

	set_t* sets;

	/* when used, count++ */
	unsigned long long count;
} cache_t;

void show_usage()
{
	printf("-h: Optional help flag that prints usage info\n");
	printf("-v: Optional verbose flag that displays trace info\n");
	printf("-s <s>: Number of set index bits (S = 2^s is the number of sets)\n");
	printf("-E <E>: Associativity (number of lines per set)\n");
	printf("-b <b>: Number of block bits (B = 2^b is the block size)\n");
	printf("-t <tracefile>: Name of the valgrind trace to replay\n");
}

cache_t* init_cache(int s, int E, int b)
{
	cache_t* cache_p = (cache_t*)malloc(sizeof(cache_t));
	int S = 1 << s;

	cache_p->s = s;
	cache_p->E = E;
	cache_p->b = b;
	cache_p->count = 0;
	cache_p->sets = (set_t*)malloc(sizeof(set_t) * S);

	int i;
	int j;
	for (i = 0; i < S; i++)
	{
		cache_p->sets[i] = (line_t*)malloc(sizeof(line_t) * E);
		/* Init each line */
		for (j = 0; j < E; j++)
		{
			cache_p->sets[i][j].valid = 0;
			cache_p->sets[i][j].count = 0;
		}
	}

	return cache_p;
}

int hit_cache(cache_t* cache_p, unsigned long long address)
{
	cache_p->count++;
	int s = cache_p->s;
	int b = cache_p->b;
	int t = BITS - s - b;
	int E = cache_p->E;
	/* Split the address */
	unsigned long long tag = TAG(address);
	int set = SET(address);

	int i;
	/* if hit */
	for (i = 0; i < E; i++)
	{
		if (cache_p->sets[set][i].valid == 1 && 
			cache_p->sets[set][i].tag == tag)
		{
			cache_p->sets[set][i].count = cache_p->count;
			return HIT;
		}
	}

	/* miss */
	for (i = 0; i < E; i++)
	{
		if (cache_p->sets[set][i].valid == 0)
		{
			cache_p->sets[set][i].count = cache_p->count;
			cache_p->sets[set][i].tag = tag;
			cache_p->sets[set][i].valid = 1;
			return MISS;
		}
	}

	/* miss and eviction */
	int min = cache_p->count;
	int min_index;
	for (i = 0; i < E; i++)
	{
		if (cache_p->sets[set][i].valid == 1 && 
			min > cache_p->sets[set][i].count)
		{
			min = cache_p->sets[set][i].count;
			min_index = i;
		}
	}

	cache_p->sets[set][min_index].count = cache_p->count;
	cache_p->sets[set][min_index].tag = tag;
	return EVICTION;
}

void parse_line(char* line, cache_t* cache_p, int verbose_flag, 
	int* hit_count, int* miss_count, int* eviction_count)
{
	int hit_flag;
	char* hit_flag_str;
	int size;
	unsigned long long address;
	/* We don't care instruction */
	if (line[0] != ' ')
	{
		return;
	}

	else if (line[1] == 'L' || line[1] == 'S')
	{
		sscanf(line + 3, "%llx,%d", &address, &size);

		if (verbose_flag == TRUE)
		{
			printf("%c %llx,%d", line[1], address, size);
		}

		hit_flag = hit_cache(cache_p, address);
		switch(hit_flag)
		{
			case MISS:
				(*miss_count)++;
				hit_flag_str = "miss";
				break;

			case HIT:
				(*hit_count)++;
				hit_flag_str = "hit";
				break;

			case EVICTION:
				(*miss_count)++;
				(*eviction_count)++;
				hit_flag_str = "eviction";
				break;
		}

		if (verbose_flag == TRUE)
		{
			printf(" %s", hit_flag_str);
		}
	}

	/* Load, then store */
	else if (line[1] == 'M')
	{
		sscanf(line + 3, "%llx,%d", &address, &size);

		if (verbose_flag == TRUE)
		{
			printf("%c %llx,%d", line[1], address, size);
		}

		int i;
		for (i = 0; i < 2; i++)
		{
			hit_flag = hit_cache(cache_p, address);
			switch(hit_flag)
			{
				case MISS:
					(*miss_count)++;
					hit_flag_str = "miss";
					break;

				case HIT:
					(*hit_count)++;
					hit_flag_str = "hit";
					break;

				case EVICTION:
					(*miss_count)++;
					(*eviction_count)++;
					hit_flag_str = "eviction";
					break;
			}

			if (verbose_flag == TRUE)
			{
				printf(" %s", hit_flag_str);
			}
		}
	}

	else
	{
		printf("Invalid line\n");
		return;
	}

	if (verbose_flag == TRUE)
	{
		printf("\n");
	}
	return;
}

void free_cache(cache_t* cache_p) 
{
	int S = 1 << (cache_p->s);
	int i;
	for (i = 0; i < S; i++)
	{
		/* free every set */
		free(cache_p->sets[i]);
	}

	/* free sets and cache itself */
	free(cache_p->sets);
	free(cache_p);
}

int main(int argc, char* argv[])
{
	char* file;
	int s = 0;
	int E = 0;
	int b = 0;
	int verbose_flag = FALSE;

	/* Parse the arg */
	int arg;
	while((arg = getopt(argc, argv, "hvs:E:b:t:")) != -1)
	{
		switch(arg) 
		{
			case 'h':
				show_usage();
				break;

			case 'v':
				verbose_flag = TRUE;
				break;

			case 's':
				s = atoi(optarg);
				break;

			case 'E':
				E = atoi(optarg);
				break;

			case 'b':
				b = atoi(optarg);
				break;

			case 't':
				file = optarg;
				break;

			default:
				show_usage();
				return -1;
		}
	}
	//printf("v=%d, s=%d, E=%d, b=%d, file=%s\n", verbose_flag, s, E, b, file);

	/* Calculate the hit,miss,eviction */
	int hit_count = 0;
	int miss_count = 0;
	int eviction_count = 0;

	/* Cache init */
	cache_t* cache_p = init_cache(s, E, b);

	/* Read lines from file and Parse them */
	FILE* file_p = NULL;
	char line[256];

	file_p = fopen(file, "r");
		/* check if the file has opened */
	if (file_p == NULL) 
	{
		printf("Can't open the file.\n");
		return -1;
	}

	while (fgets(line, 256, file_p) != NULL)
	{
		//printf("%s", line);
		parse_line(line, cache_p, verbose_flag, 
			&hit_count, &miss_count, &eviction_count);
	}
	
	/* Close the file */
	fclose(file_p);

	/* Cache free */
	free_cache(cache_p);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
