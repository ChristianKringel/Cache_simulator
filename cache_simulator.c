#include "cache.h"

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        printf("Uso: %s <nsets> <bsize> <assoc> <substituição> <flag_saida> <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }

    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char subst = argv[4][0];
    int flag = atoi(argv[5]);
    char *filename = argv[6];

    if (!isPowerOfTwo(nsets) || !isPowerOfTwo(bsize))
    {
        printf("ERRO: nsets e bsize devem ser potências de 2\n");
        return 1;
    }

    Cache *cache = createCacheWithPolicy(nsets, bsize, assoc, subst);
    if (cache == NULL)
        return 1;

    readFile(cache, filename);
    printStats(cache, flag);
    freeCache(cache);

    return 0;
}