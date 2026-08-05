#include "cachelab.h"
#include "csim.h"

struct cache_system_t g_cache_system = {0};
int hits = 0;
int misses = 0;
int evictions = 0;

// frees all dynamically allocated memory
void free_cache(void)
{
    unsigned nsets = g_cache_system.nsets;

    for (int i = 0; i < nsets; i++) {
        struct cache_line_t *curr = g_cache_system.sets[i].line;
        struct cache_line_t *prev = curr;
        while (curr != NULL) {
            curr = curr->next;
            free(prev);
            prev = curr;
        }
    }

    free(g_cache_system.sets);
    free(g_cache_system.trace_file);
}

// matches a line in a given set with a given tag
struct cache_line_t *match_line(struct cache_set_t *set, unsigned long long tag)
{
    struct cache_line_t *head = set->line;

    struct cache_line_t *curr = head;

    while (curr != NULL) {
        if (curr->valid && curr->tag == tag) return curr;
        curr = curr->next;
    }
    return NULL;
}

// allocates cache line 
struct cache_line_t *allocate_line(unsigned long long tag)
{
    struct cache_line_t *line = (struct cache_line_t*)malloc(sizeof(struct cache_line_t));
    line->next = NULL;
    line->prev = NULL;
    line->tag = tag;
    line->valid = 1;

    return line;
}

/*
 * this function simulates the instruction, prints info if verbose flag is 
 * set and updates the miss, hit, eviction counters
 */
void simulate_op(struct operation_t operation, char op)
{
    struct cache_set_t *myset = 
        &(g_cache_system.sets[operation.metadata.set_index]);

    unsigned long long tag = operation.metadata.tag;


    int is_miss = 0;
    int is_hit = 0;
    int is_evict = 0;

    /*
     * set is empty, its a miss and will have to allocate a line and initialise
     * it
     */
    if (myset->line == NULL) {
        struct cache_line_t *line = allocate_line(tag);
        myset->line = line;
        myset->end = line;
        myset->ncachelines_allocated = 1;

        is_miss = 1;
        misses++;
        if (op == 'M') hits++;
    }

    /*
     * if set has lines but tag from mem addr doesn't match with tag from any
     * existing lines so create a new line and place it to the head of linked
     * list and if new count of lines exeeds max no of lines then free and 
     * unlink a line from tail. this is a case of eviction. this way we maintain
     * a lru system
     */
    else if (!match_line(myset, tag)) {
        struct cache_line_t *line = allocate_line(tag);
        line->next = myset->line;
        myset->line->prev = line;
        myset->line = line;
        (myset->ncachelines_allocated)++;

        if (myset->ncachelines_allocated > g_cache_system.nline) {
            struct cache_line_t *last = myset->end;
            myset->end = myset->end->prev;
            if (myset->end)
                myset->end->next = NULL;
            free(last);
            (myset->ncachelines_allocated)--;
            is_evict = 1;
            evictions++;
            misses++;

            if (op == 'M') hits++;
        }
        else {
            is_miss = 1;
            misses++;
            if (op == 'M') hits++;
        }
    }

    else {
        struct cache_line_t *line = match_line(myset, tag);

        if (line->next && line->prev) {
            line->next->prev = line->prev;
            line->prev->next = line->next;
        

            line->next = myset->line;
            line->prev = NULL;
            myset->line->prev = line;
            myset->line = line;
        }

        else if (!line->next && line->prev) {
            line->prev->next = NULL;
            myset->end = line->prev;
            line->next = myset->line;
            line->prev = NULL;
            myset->line->prev = line;
            myset->line = line;
        }

        is_hit = 1;
        hits++;
        if (op == 'M') hits++;
    }
    

    if (g_cache_system.is_verbose && op != 'M') {
        printf("%c %llx,%d", operation.op, operation.mem_addr, operation.size);
        if (is_miss) printf(" miss\n");
        if (is_evict) printf(" miss eviction\n");
        if (is_hit) printf(" hit\n");
    }

    if (g_cache_system.is_verbose && op == 'M') {
        printf("%c %llx,%d", operation.op, operation.mem_addr, operation.size);
        if (is_miss) printf(" miss hit\n");
        if (is_evict) printf(" miss eviction hit\n");
        if (is_hit) printf(" hit hit\n");
    }

    return;
}

//retrives metadata from the mem addr of operation and sets the metadata struct
void retrieve_metadata(struct operation_t *operation)
{
    unsigned long long addr = operation->mem_addr;

    unsigned long long block_offset = addr & (g_cache_system.block_size - 1);
    addr = addr >> g_cache_system.nblock_offset_bits;
    unsigned long long set_index = addr & (g_cache_system.nsets - 1);
    unsigned long long tag = addr >> g_cache_system.nset_index_bit;

    operation->metadata.block_offset = block_offset;
    operation->metadata.set_index = set_index;
    operation->metadata.tag = tag;

    return;
}

//parse a line from trace file to fill the operation_t struct
struct operation_t parse_line(char *line)
{
    char op;
    unsigned long long mem_addr;
    int size;

    if (sscanf(line, " %c %llx,%d", &op, &mem_addr, &size) != 3)
        perror("Failure while parsing lines from the trace file");

    struct operation_t operation = {{0}, mem_addr, size, op};
    return operation;
}

/*
 * start cache simulation by reading all the lines from trace file and parsing
 * it. 
 */
void start_simulation(void)
{
    g_cache_system.file = fopen(g_cache_system.trace_file, "r");

    FILE *file = g_cache_system.file;
    if (file == NULL) {
        perror("Error opening the trace file");
        exit(-1);
    }

    char *line = NULL;
    size_t size = 0;
    ssize_t read;

    while ((read = getline(&line, &size, file)) != -1) {
        struct operation_t operation = parse_line(line);
        //printf("op: %c, addr: %llx, size: %d\n", operation.op, operation.mem_addr, operation.size);
        retrieve_metadata(&operation);

        char op = operation.op;
        switch (op) {
            case 'L':
                simulate_op(operation, 'L');
                break;
            case 'S':
                simulate_op(operation, 'S');
                break;
            case 'M':
                simulate_op(operation, 'M');
                break;
            case 'I':
                break;
            default:
                fprintf(stderr, "invalid operation: %c\n", op);
                exit(-1);
        }
    }
    free(line);
    fclose(file);
}

/* initialise the cache system: allocates memory for sets and initialise the 
 * nines to null
 */
void init_cache(void)
{
    unsigned nsets = g_cache_system.nsets;
    
    g_cache_system.sets = (struct cache_set_t*)calloc(nsets, sizeof(struct cache_set_t));

    struct cache_set_t *sets = g_cache_system.sets;
    for (int i = 0; i < nsets; i++) {
        sets[i].line = NULL;
        sets[i].end = NULL;
        sets[i].ncachelines_allocated = 0;
    }

    return;
}

// power function for int data types
int ipow(int base, unsigned exp)
{
    if (exp == 1) return base;
    int result = 1;

    // for squaring
    if (exp == 2) {
        return base * base;
    }

    for (int i = 0; i < exp; i++) {
        result *= base;
    }

    return result;
}

/*
 * parse the cli args using getopt() and populate the cache_system_t
 * struct
 */
void parse_args(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "-h")) {
        printf("%s", usage_message);
        exit(-1);
    }

    if (argc < 9 || argc > 10) {
        printf("%s", missing_cli_args);
        printf("%s", usage_message);
        exit(-1);
    }

    int opt;
    unsigned is_verbose, s, E, b;
    char trace_file[32];
    char *space;
    size_t trace_file_namelen;
    size_t trace_file_buflen = sizeof(trace_file);

    is_verbose = 0;
    while ((opt = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch (opt) {
            case 'v':
                is_verbose = 1;
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
                space = strchr(optarg, ' ');
                trace_file_namelen = space ? (size_t)(space - optarg)
                    : strlen(optarg);
                if (trace_file_namelen > trace_file_buflen)
                    trace_file_namelen = trace_file_buflen - 1;
                memcpy(trace_file, optarg, trace_file_namelen);
                trace_file[trace_file_namelen] = '\0';
                break;
            default:
                printf("%s", missing_cli_args);
                printf("%s", usage_message);
                break;
        }
    }

    g_cache_system.nsets = ipow(2, s);
    g_cache_system.nline = E;
    g_cache_system.block_size = ipow(2, b);
    g_cache_system.nset_index_bit = s;
    g_cache_system.nblock_offset_bits = b;
    g_cache_system.trace_file = strdup(trace_file);
    g_cache_system.is_verbose = is_verbose;

    //for debug use. it prints all the cli args parsed
    //printf("s: %d, E: %d, b: %d, t: %s, v: %d\n", s, E, b, trace_file, is_verbose);
}

int main(int argc, char **argv, char **envp)
{
    parse_args(argc, argv);
    init_cache();
    start_simulation();

    free_cache();

    printSummary(hits, misses, evictions);
    return 0;
}
