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
