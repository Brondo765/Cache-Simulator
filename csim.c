#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Stores relative info about a cache */
typedef struct {

	int s; 		// Number of set index bits
	int E; 		// Number of lines/set
	int b; 		// Number of block offset bits
	int S; 		// S = 2^s (Number of sets)
	int B; 		// B = 2^b (Block size in bytes)
	char *p;	// Policy for evictions
	char *file; 	// Trace file we are testing our Csim on

} cache_makeup;

/* Stores relative info about a line */
typedef struct {

	int valid; 		// Bit(s) represents whether cache is valid or not
	unsigned long long tag; // Tag which will be put into the cache if empty/to check for hits in line if valid
	int num_accesses; 	// Count of number of times that line was accessed in the cache (replacement policy)
	int timer;		// Timer is used for LFU evictions which keeps track of which line in the set hasn't been accessed in some time

} line_makeup;

/* Stores relative info about a set */
typedef struct {

	line_makeup *lines;	// References the lines in that particular set
	int line_count;		// Number of lines in a set

} set_makeup;

/* Stores relative info about the whole cache */
typedef struct {

	set_makeup *sets; 	// References the sets in the total cache
	int hits;		// Number of cache hits (valid bit and tag in cache)
	int misses;		// Number of misses (Tagged address not found in cache set line)
	int evicts;		// Number of evictions (Removal based on policy)
	int set_count;		// Number of sets in cache
	int line_count;		// Number of lines in the cache
	int block_size;		// Total block size

} cache_itself;

/* Initilizes the cache based on command arg input and returns the cache back to main() */
cache_itself makeCache(cache_makeup parameters, cache_itself cache) {
    	set_makeup set;
    	line_makeup line;

	parameters.S = (1 << parameters.s);
	parameters.B = (1 << parameters.b);

	cache.set_count = parameters.S;
	cache.line_count = parameters.E;
	cache.block_size = parameters.B;
        
	cache.sets = (set_makeup *) malloc(parameters.S * sizeof(set_makeup));

	for (int i = 0; i < parameters.S; i++) {
		set.lines = (line_makeup *) malloc(parameters.E * sizeof(line_makeup));
		cache.sets[i] = set;
		for (int j = 0; j < parameters.E; j++) {
			line.valid = 0;
			line.tag = 0;
			line.num_accesses = 0;
			line.timer = 0;
			set.lines[j] = line;
		}
	}
	return cache;
}

/* When finished with the simulation free any allocation in the Heap */
void clearCache(cache_itself cache) {
	int i;
	for (i = 0; i < cache.set_count; i++) {
		free(cache.sets[i].lines);
	}

	free(cache.sets);
	return;
}

/* Finds the line with the smallest tag in the set for removal */
int findSmallestTagIndex(set_makeup curr_set, cache_makeup parameters, cache_itself cache) {
	unsigned long long min_tag_index = curr_set.lines[0].tag;
    	int index = 0;
    	int i;
    	for (i = 0; i < parameters.E; i++) {
        	if (min_tag_index > curr_set.lines[i].tag) {
            	min_tag_index = curr_set.lines[i].tag;
            	index = i;
        	}
    	}
    	return index;
}

/* Finds the line with the largest tag in the set for removal */
int findLargestTagIndex(set_makeup curr_set, cache_makeup parameters, cache_itself cache) {
    	unsigned long long max_tag_index = curr_set.lines[0].tag;
    	int index = 0;
   	int i;
    	for (i = 0; i < parameters.E; i++) {
        	if (max_tag_index < curr_set.lines[i].tag) {
            	max_tag_index = curr_set.lines[i].tag;
            	index = i;
        	}
	}
    	return index;
}

/* Finds the line who was least recently accesses in the set */
int findLeastRecentlyUsedIndex(set_makeup curr_set, cache_makeup parameters, cache_itself cache) {
	int least_recent_used = curr_set.lines[0].num_accesses;
	int index = 0;	
	int i;
	for (i = 0; i < parameters.E; i++) {
		if (least_recent_used < curr_set.lines[i].num_accesses) {
			least_recent_used = curr_set.lines[i].num_accesses;
			index = i;
		}
	}
	
	return index;
}

/* Finds the line who has been least used in the set */
int findLeastFrequentlyUsedIndex(set_makeup curr_set, cache_makeup parameters, cache_itself cache) {
	int curr_LFU = curr_set.lines[0].timer;
	int second_LFU;
	unsigned long long tag1 = curr_set.lines[0].tag;
	unsigned long long tag2;
	unsigned long long tag3;
	int first_encounter_index = 0;
	int second_encounter_index;
	int third_encounter_index;
	int index = 0;
	int count = 0;
	int i;
	for (i = 1; i < parameters.E; i++) {
		if (curr_set.lines[i].timer == curr_LFU) {
			count++;
			second_encounter_index = i;
			tag2 = curr_set.lines[i].tag;
			second_LFU = curr_set.lines[i].timer;
		}

		if (curr_set.lines[i].timer < curr_LFU) {
			tag1 = curr_set.lines[i].tag;
			first_encounter_index = i;
			curr_LFU = curr_set.lines[i].timer;
			index = i;
		}

		if (curr_set.lines[i].timer == second_LFU && curr_set.lines[i].timer == curr_LFU) {
            		third_encounter_index = i;
            		tag3 = curr_set.lines[i].tag;
        	}
	}
	if (count == 1) {
		if (tag1 > tag2) {
			index = second_encounter_index;
		}
		else if (tag2 > tag1) {
			index = first_encounter_index;
		}
	} if (count >= 2) {
		if (tag1 < tag2 && tag1 < tag3) {
			index = first_encounter_index;
		}
		else if(tag2 < tag1 && tag2 < tag3) {
			index = second_encounter_index;
		} 
		else if(tag3 < tag1 && tag3 < tag2) {
			index = third_encounter_index;
		}
	}
	// printf("TOTAL COUNT WAS: %d\n", count);	
	return index;
}	

/* 
Simulation is run checking for hits, misses, and evictions in the cache.

If a set index is found and the line who potentially matches the tag for the address passed in will
be marked as a hit.

If the set has lines that are empty then the tag of the checked address is stored in the set specified
by index.

If there is no space to store another tag in the set, then an eviction occurs based on removal policy
and the new tag overwrites the line deemed fit for removal. 
*/
cache_itself runSimulation(unsigned long long address, cache_makeup parameters, cache_itself cache) {
	unsigned long long tag, set_index;
	int evict_index = 0;
	int empty_index = 0; 
	int empty_flag = 0;
	int hit_flag = 0;	
	set_makeup set;

	set_index = (address >> parameters.b) & ((1 << parameters.s) - 1);
	tag = (address >> (parameters.b + parameters.s)) & ((1 << (63 - parameters.b - parameters.s)) - 1);
	set = cache.sets[set_index];

	for (int i = 0; i < parameters.E; i++) {
		if ((set.lines[i].valid == 1) && (set.lines[i].tag == tag)) {
			// printf("HIT ");
			set.lines[i].num_accesses = 0;
			set.lines[i].timer++;
			hit_flag = 1;
			cache.hits++;
		}

		else if (set.lines[i].valid == 0) { 
			empty_flag = 1;
			empty_index = i;

		} else {
			set.lines[i].num_accesses++;
		}

	}
		
	if (hit_flag == 0) {
		if (empty_flag == 1) {
        		// printf("MISS ");
           	 	set.lines[empty_index].num_accesses = 0;
			set.lines[empty_index].timer = 0;
            		set.lines[empty_index].valid = 1;
            		set.lines[empty_index].tag = tag;
            		cache.misses++;
       		 }

		else if (empty_flag == 0) {
			cache.misses++;
			cache.evicts++;
			if (strcmp(parameters.p, "lru") == 0) {
				// printf("MISS AND EVICT ");
				evict_index = findLeastRecentlyUsedIndex(set, parameters, cache);
                		set.lines[evict_index].tag = tag;
				set.lines[evict_index].num_accesses = 0;
				set.lines[evict_index].timer = 0;

			}
			else if (strcmp(parameters.p, "lfu") == 0) {
                		// printf("MISS AND EVICT ");
				evict_index = findLeastFrequentlyUsedIndex(set, parameters, cache);
                		set.lines[evict_index].tag = tag;
                		set.lines[evict_index].num_accesses = 0;
				set.lines[evict_index].timer = 0;
				
			}
			else if (strcmp(parameters.p, "low") == 0) {
				// printf("MISS AND EVICT ");
				evict_index = findSmallestTagIndex(set, parameters, cache);
				set.lines[evict_index].tag = tag;
				set.lines[evict_index].num_accesses = 0;
				set.lines[evict_index].timer = 0;
			}
			else if (strcmp(parameters.p,"hig") == 0) {
				// printf("MISS AND EVICT ");
				evict_index = findLargestTagIndex(set, parameters, cache);
				set.lines[evict_index].tag = tag;
				set.lines[evict_index].num_accesses = 0;
				set.lines[evict_index].timer = 0;
			}
		} 
	}
	evict_index = 0;
    	empty_index = 0;
    	empty_flag = 0;
    	hit_flag = 0;
	return cache;
}

/* 
Parses trace files passed via command line. Stores the address, the operation: Modify, Load, or Store
and the size of the request to the cache.
*/
cache_itself printFileTraceVerbose(cache_makeup parameters, cache_itself cache) {
	char *file = parameters.file;
	FILE *file_stream;
    	file_stream = fopen(file, "r");

    	if (file_stream == NULL) {
        	printf("Please make sure the file path is correct to create a stream.\n");
    	}

    	char operation;
    	unsigned long long address;
    	int size;
    	// %c = M, L, S
    	// %llx = Memory address
    	// %x = size
	// The instruction or operation as I call it will be a space followed by a char, long long, and int
	while (fscanf(file_stream, " %c %llx,%x", &operation, &address, &size) > 0) {
        	switch(operation) {
		    	case 'I':
				break;

            		case 'M':
				cache = runSimulation(address, parameters, cache);
			 	// printf("After first: M ");
				cache = runSimulation(address, parameters, cache);
				// printf("After second: ");
			 	// printf("%c, %llx,%x\n", operation, address, size);
                		break;

            		case 'L':
				cache = runSimulation(address, parameters, cache);
			 	// printf("%c, %llx,%x\n", operation, address, size);
                		break;

            		case 'S':
				cache = runSimulation(address, parameters, cache);
				// printf("%c, %llx,%x\n", operation, address, size);
                		break;

            		default:
                		break;
		}
	}
	
	fclose(file_stream);
	return cache;
}

// Command line parse:
// Usage: ./csim [-hv] -s <s> -E <E> -b <b> -p <policy> -t <tracefile>

// ./csim = this file
// h = help flag
// v = verbose mode display trace info
// s = Set index bits
// E = Associativity (lines/set)
// b = block bits (aka block size)
// p = policy to use (lru, lfu, min, hig)
// t = trace file we want to test our cache on

/* Prints the command line structure for arg input */
void usage() {
    printf("Example Template: ./csim [<-hv>] -s <s> -E <E> -b <b> -p <policy> -t <tracefile>\n");
    printf("-h: Optional help flag that prints usage info (this message)\n");
    printf("-v: Optional verbose flag that displays trace info\n");
    printf("-s <s>: Number of set index bits (S=2s is the number of sets)\n");
    printf("-E <E>: Associativity (number of lines per set)\n");
    printf("-b <b>: Number of block bits (B=2b is the block size)\n");
    printf("-p <policy>: Replacement policy (one of lru, lfu, low, or hig)\n");
    printf("-t <tracefile>: Name of the valgrind trace to replay");
}

/* 
Initial parsing of commands occurs here, the number of sets, lines, and block offset are parsed and stored
in the structs at the top of the file. 
*/ 
int main(int argc, char *argv[])
{
    int choice;
    int file_flag = 0; 
    int index_bits_flag = 0; 
    int lines_per_set_flag = 0;
    int block_size_flag = 0;
    int p_flag = 0;
    cache_itself cache;
    cache_makeup cache_parameters;

    /* The getopt() function parses the command-line arguments. Its arguments argc and argv are the
     * argument count and array as passed to the main() function on program invocation. An element of argv that starts with '-'
     * (and is not exactly "-" or "--") is an option element. The characters of this element (aside from the initial '-')
     * are option characters. If getopt() is called repeatedly, it returns successively each of the option characters from
     * each of the option elements.
     */

    /*
     * The atoi() functions returns the converted integral number as an int value. If no valid conversion could be performed,
     * it returns zero.
     */
    while ((choice = getopt(argc, argv, "hvs:E:b:p:t:")) != -1) {
        switch(choice) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;

			case 'v':
				break;

			case 's':
				index_bits_flag = 1; 
				cache_parameters.s = atoi(optarg);
				break;

			case 'E':
				lines_per_set_flag = 1;
				cache_parameters.E = atoi(optarg);
				break;

			case 'b':
				block_size_flag = 1;
				cache_parameters.b = atoi(optarg);
				break;

			case 'p':
				p_flag = 1;
				cache_parameters.p = optarg;
				break;

			case 't':
				file_flag = 1;
				cache_parameters.file = optarg;
				break;

			default:
				printf("No argument encountered in entry");
				exit(EXIT_FAILURE);
		}
    }

    if (index_bits_flag == 0 || lines_per_set_flag == 0 || block_size_flag == 0 || p_flag == 0 || file_flag == 0) {
        printf("Need all values to form a proper cache/check for correctness\n");
		exit(EXIT_FAILURE);
    }
	    
    // Make the cache
    cache = makeCache(cache_parameters, cache);

    // Set cache hits, misses, and evicts to 0 before entering simulation
    cache.hits = 0;
    cache.misses = 0;
    cache.evicts = 0;

    // Go Through trace file to read whether M, L, or S operation and simulate cache hits and misses through
    // runSimulation function
    cache = printFileTraceVerbose(cache_parameters, cache);

    // Finally we print the summary of hits, misses, and evicts
    printSummary(cache.hits, cache.misses, cache.evicts);

	// Clears the cache since we are finished with the simulation
    clearCache(cache);

    return 0;
}
