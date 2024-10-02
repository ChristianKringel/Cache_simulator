#include "cache.h"

// func para testar se esta em potencia de dois
static bool powerOfTwo(const unsigned int value)
{
    return (value & (value - 1)) == 0;
}

// inicializacao da cache
Cache *initializeCache(Cache *cache, unsigned int associativity, unsigned int block_size, unsigned int nsets, char substitution_policy)
{
    cache = (Cache *)malloc(sizeof(Cache)); // aloca memoria para a cache
    if (cache == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para a cache\n");
        return NULL;
    }

    cache->misses = (CacheMisses *)malloc(sizeof(CacheMisses)); // aloca memoria para os misses
    if (cache->misses == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para os misses\n");
        return NULL;
    }

    cache->hits = (CacheHits *)malloc(sizeof(CacheHits)); // aloca memoria para os hits
    if (cache->hits == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para os hits\n");
        free(cache->misses);
        free(cache);
        return NULL;
    }

    cache->accesses = (CacheTotalAccesses *)malloc(sizeof(CacheTotalAccesses)); // aloca memoria para accesses
    if (cache->accesses == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para os acessos\n");
        free(cache->hits);
        free(cache->misses);
        free(cache);
        return NULL;
    }

        /* Inicializa todos os dados, caso contrario pode dar BO*/
        cache->associativity = associativity;
        cache->block_size = block_size;
        cache->nsets = nsets;
        cache->substitution_policy = substitution_policy;
        cache->misses->totalMisses = 0;
        cache->misses->capacity = 0;
        cache->misses->compulsory = 0;
        cache->misses->conflict = 0;
        cache->hits->hits = 0;
        cache->accesses->totalAccesses = 0;

        if (!powerOfTwo(nsets))
        {
            printf("ERRO: o numero de conjuntos deve ser uma potencia de 2\n");
            free(cache);
            return NULL;
        }

        if (!powerOfTwo(block_size))
        {
            printf("ERRO: tamanho do bloco deve ser uma potencia de 2\n");
            free(cache);
            return NULL;
        }

        cache->cache_address = (CacheAddress *)malloc(nsets * associativity * sizeof(CacheAddress));
        // aloca memoria para o Adress usando um calculo do "tamanho total" da cache
        if (cache->cache_address == NULL)
        {
            printf("ERRO: Problema ao alocar memoria para os endereços da cache\n");
            free(cache);
            return NULL;
        }

        // Inicializa a cache
        for (int i = 0; i < nsets * associativity; i++)
        {
            cache->cache_address[i].tag = 0;
            cache->cache_address[i].valid = false;
            cache->cache_address[i].itWasAdded = 0;
            cache->cache_address[i].lastAccess = 0;
        }

        return cache;
    }

    Cache *freeCache(Cache * cache)
    {
        if (cache != NULL)
        {
            free(cache);
            cache = NULL;
        }
        return cache;
    }

    void readFile(Cache * cache, const char *file)
    {
        FILE *fp = fopen(file, "rb");
        int endereco;

        if (!fp)
        {
            printf("ERRO: Problema ao abrir o arquivo %s\n", file);
            return;
        }

        while (fread(&endereco, sizeof(int), 1, fp) == 1)
        {

            endereco = __builtin_bswap32(endereco);
            // printf("Endereco: %d\n", endereco);
            processAddress(cache, endereco);
        }
        fclose(fp);
    }

    void processAddress(Cache * cache, int address)
    {
        // calculo padrao usando log
        unsigned int offset_bits = log2(cache->block_size);
        unsigned int index_bits = log2(cache->nsets);

        uint32_t tag = address >> (offset_bits + index_bits);
        int medio = pow(2, index_bits); // somente teste
        uint32_t index = (address >> offset_bits) & (medio - 1);
        // Mascara, fazendo um AND bit a bit
        int lineIndex = index * cache->associativity; // primeiro indice de uma linha do conjunto
        // esse indice eh a posicao correspondente ao conjunto atual

        // Ve se eh hit
        bool isHit = false;

        for (int i = lineIndex; i < lineIndex + cache->associativity; i++) // comeca em lineIndex pq eh o primeiro indice do conjunto
        // percorre ate lineIndex + assoc por causa das n-vias
        // como cada conjunto pode ter varias vias, testamos todas
        {
            if (cache->cache_address[i].valid && cache->cache_address[i].tag == tag)
            { // se for valido e a tag for igual, eh hit
                isHit = true;
                cache->hits->hits++;
                if (cache->substitution_policy == 'L') // incrementa o tempo de uso (para lru)
                    cache->cache_address[i].lastAccess = cache->accesses->totalAccesses;

                break;
            }
        }

        if (!isHit)
        {
            // se n eh hit conta como miss
            cache->misses->totalMisses++;

            // Procura por uma linha vazia no conjunto
            int emptyLine = -1;
            for (int i = lineIndex; i < lineIndex + cache->associativity; i++)
            {
                if (!cache->cache_address[i].valid)
                {
                    emptyLine = i;
                    break;
                }
            }

            // se nao tiver linha vazia usa o replace
            int indexReplace = (emptyLine != -1) ? emptyLine : indexForReplace(cache, index);

            // se for linha vazia eh compulsorio
            if (emptyLine != -1)
            {
                cache->misses->compulsory++;
            }
            else
            {
                // se a cache esta cheia eh de capacidade
                if (cacheIsFull(cache))
                    cache->misses->capacity++;
                else
                    // se nao for nenhum dos outros, logo eh conflito
                    cache->misses->conflict++;
            }

            // Se a politica de substituicao for FIFO ou LFU, atualiza o tempo de uso
            if (cache->substitution_policy == 'F')
                cache->cache_address[indexReplace].itWasAdded = cache->accesses->totalAccesses;

            else if (cache->substitution_policy == 'L')
                cache->cache_address[indexReplace].lastAccess = cache->accesses->totalAccesses;

            // Substitui a linha
            cache->cache_address[indexReplace].tag = tag;
            cache->cache_address[indexReplace].valid = true;
        }
        // Incrementa o total de acessos
        cache->accesses->totalAccesses++;
    }

    // somente pra organizar o cdg
    int indexForReplace(Cache * cache, int index)
    {
        // printf("\n\nPOLITICA DE SUBSTITUICAO:%c\n", cache->substitution_policy);
        if (cache->substitution_policy == 'L')
        {
            return LRU(cache, index);
        }
        else if (cache->substitution_policy == 'R')
        {
            return RANDOM(cache, index);
        }
        else if (cache->substitution_policy == 'F')
        {
            return FIFO(cache, index);
        }
        else
        {
            printf("Politica de substituicao invalida.\n");
            exit(EXIT_FAILURE);
        }
    }

    // funcao para calcular o endereco usando algoritmo aleatorio
    // explicacoes melhores sobre pq da formula abaixo
    int RANDOM(Cache * cache, int index)
    {
        // testei com cache->associativity + index * cache->associativity
        // testei com cache->associativity * cache->associativity
        // e testei com cache->associativity * index
        return rand() % cache->associativity + (index * cache->associativity); // foi o mais perto dos resultados esperados
        // nao buga com o vortex
        // return rand() % cache->associativity + ( cache->associativity );
    }
    /* Funcao para calcular o endereco usando fifo */
    int FIFO(Cache * cache, int index)
    {
        int indexReplace = index * cache->associativity;
        int indexFifo = indexReplace;
        int menor = cache->cache_address[indexReplace].itWasAdded;               // incia o menor no primeiro valor
        for (int i = indexReplace; i < indexReplace + cache->associativity; i++) // percorre todas as linhas dentro do conjunto
        {
            if (cache->cache_address[i].itWasAdded < menor) // compara os tempos para encontrar o menor (a linha menos recentemente usada)
            {
                menor = cache->cache_address[i].itWasAdded; // atualiza o menor valor
                indexFifo = i;                              // atualiza o indice da linha menos recentemente usada
            }
        }
        return indexFifo;
    }

    /* ##### */
    /* Funcoes de print conforme a flag*/
    void printFlag0(Cache * cache)
    {
        printf("Total de acessos: %u\n", cache->accesses->totalAccesses);
        printf("Total de hits: %u\n", cache->hits->hits);
        printf("Total de misses: %u\n", cache->misses->totalMisses);
        printf("Taxa de hits: %.4f\n", (float)cache->hits->hits / cache->accesses->totalAccesses);
        printf("Taxa de misses: %.4f\n", (float)cache->misses->totalMisses / cache->accesses->totalAccesses);
        printf("Taxa de misses compulsorios: %.2f\n", (float)cache->misses->compulsory / cache->misses->totalMisses);
        printf("Taxa de misses por capacidade: %.2f\n", (float)cache->misses->capacity / cache->misses->totalMisses);
        printf("Taxa de misses por conflito: %.2f\n", (float)cache->misses->conflict / cache->misses->totalMisses);
    }

    void printFlag1(Cache * cache)
    {
        printf("%u %.4f %.4f %.2f %.2f %.2f\n", cache->accesses->totalAccesses, (float)cache->hits->hits / cache->accesses->totalAccesses,
               (float)cache->misses->totalMisses / cache->accesses->totalAccesses, (float)cache->misses->compulsory / cache->misses->totalMisses,
               (float)cache->misses->capacity / cache->misses->totalMisses, (float)cache->misses->conflict / cache->misses->totalMisses);
    }

    // funcao usada no setIsFull
    unsigned int getIndexForCheckFull(unsigned int set, unsigned int way, unsigned int associativity)
    // calcula o indice correto para verificar se a cache esta cheia
    // seria mais simples e intuitivo se a cache fosse multidimensional (vetor de vetores)
    // como a cache esta organizada em multiplos conjuntos e cada conjunto com varias vias (assoc)
    // o correto eh accessar uma entrada a partir do calculo com set * associativity + way
    {
        return set * associativity + way;
    }

    // Função para ver se esta cheia a cache
    bool cacheIsFull(Cache * cache)
    {
        // como dito acima essa func seria mais simples se a cache fosse multidimensional
        // a ideia eh percorrer todas as vias de todos os conjuntos
        // pq queremos ver se toda a cache esta cheia
        for (unsigned int i = 0; i < cache->nsets; i++) // primeiro percorre os conjuntos
        {
            for (unsigned int j = 0; j < cache->associativity; j++) // em cada conjunto percorre todas as vias
            {
                if (cache->cache_address[getIndexForCheckFull(i, j, cache->associativity)].valid == false)
                    return false; // caso encontre uma via nao usada, retorna false (nao esta vazia)
            }
        }
        return true; // Se todos sao validos, a cache esta cheia e portanto retorna true
    }

    // funcao para calcular o LRU
    int LRU(Cache * cache, int index)
    {
        int indexReplace = index * cache->associativity;
        int indexLru = indexReplace;
        int menor = cache->cache_address[indexReplace].lastAccess; // incia o menor no primeiro valor
                                                                   // menor valor de tempo de uso
        // toda vez que um endereco eh acessado, o tempo de uso eh incrementado
        // independente de ser hit ou miss

        // Percorre todas as linhas dentro do conjunto para encontrar a menos recentemente usada
        for (int i = indexReplace; i < indexReplace + cache->associativity; i++)
        {
            // Compara os tempos para encontrar o menor (a linha menos recentemente usada)
            if (cache->cache_address[i].lastAccess < menor)
            {
                menor = cache->cache_address[i].lastAccess;
                indexLru = i; // atualiza o índice da linha menos recentemente usada
            }
        }

        return indexLru; // retorna o índice da linha que deve ser substituída
    }