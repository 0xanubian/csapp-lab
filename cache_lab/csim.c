#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"
#include "csim.h"

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
    int is_verbose, s, E, b;
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

    printf("s: %d, E: %d, b: %d, t: %s, v: %d\n", s, E, b, trace_file, is_verbose);
}

int main(int argc, char **argv, char **envp)
{
    parse_args(argc, argv);

    printSummary(0, 0, 0);
    return 0;
}
