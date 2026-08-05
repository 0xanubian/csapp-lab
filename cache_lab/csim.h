#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct metadata_t {
    unsigned long long block_offset;
    unsigned long long set_index;
    unsigned long long tag;
};

struct operation_t {
    struct metadata_t metadata;
    unsigned long long mem_addr;
    int size;
    char op;
};

struct cache_line_t {
    struct cache_line_t *next;  // a pointer to the next line
    struct cache_line_t *prev;  // a pointer to the previous line
    //unsigned char *cache_block; // an array of bytes for mem block in cache
    int valid;                  // valid bit
    int tag;                    // tag to match with the tag from memory addr
};

struct cache_set_t {
    struct cache_line_t *line;  // a pointer to the head of linked list of cache lines
    struct cache_line_t *end;   // a pointer to the tail of linked list of cache lines
    unsigned ncachelines_allocated;       // index of current set
};

struct cache_system_t {
    struct cache_set_t *sets;   // array of sets allocated dynamically
    char *trace_file;
    FILE *file;

    unsigned nsets;                  // 2^s
    unsigned nline;                  // E
    unsigned block_size;             // 2^b
    unsigned nset_index_bit;         // s
    unsigned nblock_offset_bits;     // b

    int is_verbose;
};
static const char *missing_cli_args = "./csim: Missing required command line argument\n";

static const char *usage_message = "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
                            "Options:\n"
                            "  -h         Print this help message.\n"
                            "  -v         Optional verbose flag.\n"
                            "  -s <num>   Number of set index bits.\n"
                            "  -E <num>   Number of lines per set.\n"
                            "  -b <num>   Number of block offset bits.\n"
                            "  -t <file>  Trace file.\n"
                            "\nExamples:\n"
                            "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
                            "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";
