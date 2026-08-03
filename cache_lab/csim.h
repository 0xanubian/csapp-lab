struct cache_line_t {
    struct cache_line_t *next;  // a pointer to the next line
    unsigned char *cache_block; // an array of bytes for mem block in cache
    int valid;                  // valid bit
    int tag;                    // tag to match with the tag from memory addr
};

struct cache_set_t {
    struct cache_line_t *line;  // a pointer to the linked list of cache lines
    unsigned cache_index;       // index of current set
};

struct cache_system_t {
    struct cache_set_t *sets;   // array of sets allocated dynamically

    unsigned nsets;                  // 2^s
    unsigned nline;                  // E
    unsigned block_size;             // 2^b
    unsigned nset_index_bit;         // s
    unsigned nblock_offset_bits;     // b

    int is_verbose;
    char *trace_file;
};
static const char *missing_cli_args = "./csim-ref: Missing required command line argument\n";

static const char *usage_message = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
                            "Options:\n"
                            "  -h         Print this help message.\n"
                            "  -v         Optional verbose flag.\n"
                            "  -s <num>   Number of set index bits.\n"
                            "  -E <num>   Number of lines per set.\n"
                            "  -b <num>   Number of block offset bits.\n"
                            "  -t <file>  Trace file.\n"
                            "\nExamples:\n"
                            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
                            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";
