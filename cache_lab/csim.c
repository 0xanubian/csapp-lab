#include "cachelab.h"
#include "csim.h"

struct cache_system_t g_cache_system = {0};

/*
 * load function
 */
void load(struct operation_t operation);

/*
 * store function
 */
void store(struct operation_t operation);

/*
 * modify function
 */
void modify(struct operation_t operation);

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
                load(operation);
                break;
            case 'S':
                store(operation);
                break;
            case 'M':
                modify(operation);
                break;
            default:
                fprintf(stderr, "invalid operation: %c\n", op);
                exit(-1);
        }

    }
}

/* initialise the cache system: allocates memory for sets and initialise the 
 * nines to null
 */
void init_cache(void)
{
    unsigned nsets = g_cache_system.nsets;
    
    g_cache_system.sets = calloc(nsets, sizeof(struct cache_set_t));

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

    for (int i = 1; i < exp; i++) {
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

    printSummary(0, 0, 0);
    return 0;
}
