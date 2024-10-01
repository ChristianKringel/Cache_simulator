#include "cache.h"

int main(int argc, char *argv[])
{
    Cache *cache;
    if (argc < 7)
    {
        printf("Uso: %s <nsets> <bsize> <assoc> <subs> <flag_saida> <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }
    int nsets = 0;
    int block_size = 0;
    int associativity = 0;
    char substitution_policy;
    int exit_flag = 0;

    nsets = atoi(argv[1]);
    block_size = atoi(argv[2]);
    associativity = atoi(argv[3]);
    substitution_policy = toupper(argv[4][0]);
    exit_flag = atoi(argv[5]);
    char *trace_file = argv[6];

    /* testando os parametros passados na chamada do codigo*/
    /*
    if (debug)
    {
        printf("nsets: %d\n", nsets);
        printf("bsize: %d\n", block_size);
        printf("assoc: %d\n", associativity);
        printf("sub_policy: %c\n", substitution_policy);
        printf("exit_flag: %d\n", exit_flag);
        printf("trace_file: %s\n", trace_file);
    }
    */
    cache = initializeCache(cache, associativity, block_size, nsets, substitution_policy);

    readFile(cache, trace_file);

    if (exit_flag == 0)
    {
        printFlag0(cache);
    }
    else if (exit_flag == 1)
    {
        printFlag1(cache);
    }
    free(cache);
    return 0;
}