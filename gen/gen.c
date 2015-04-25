#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SMALL_PRIMES_SIZE 10000
#define SMALL_PRIMES_MAX 0x10000L

#define FILECOUNT_MAX 10000
#define PATH_MAX_LENGTH 50
#define FILENAME_SIZE 100
#define FILENUM_SIZE 5

#define DEFAULT_FILECOUNT 4
#define DEFAULT_NUMCOUNT 100
#define DEFAULT_PRIMECOUNT 50
#define DEFAULT_PRIMEMAX 0x100000000L
#define DEFAULT_NUMMAX 0xffffffffffffffffL

#define ARGNAME_FILECOUNT "--file-count"
#define ARGNAME_NUMCOUNT "--num-count"
#define ARGNAME_PRIMECOUNT "--prime-count"
#define ARGNAME_PRIMEMAX "--prime-max"
#define ARGNAME_NUMMAX "--num-max"
#define ARGNAME_DEBUG "--debug"

int small_primes_num = 0;
uint64_t small_primes[SMALL_PRIMES_SIZE];
bool debug = false;

void gen_small_primes()
{
    // Sieve array
    bool prime[SMALL_PRIMES_MAX];
    for (uint64_t i = 2; i < SMALL_PRIMES_MAX; i++)
        prime[i] = true;
    
    // Straightforward sieve of Erathosthenes
    for (uint64_t i = 2; i < SMALL_PRIMES_MAX; i++)
    {
        if (prime[i])
        {
            small_primes[small_primes_num] = i;
            small_primes_num++;
            
            for (uint64_t j = i; i * j < SMALL_PRIMES_MAX; j++)
                prime[i*j] = false;
        }
    }
}

bool is_prime(uint64_t n)
{
    if (n < 2)
        return false;
    for (int i = 0; i < small_primes_num
            && small_primes[i] * small_primes[i] <= n; i++)
    {
        if (n % small_primes[i] == 0)
            return false;
    }
    return true;
}

bool is_distinct(uint64_t p, uint64_t *prev, int num_prev)
{
    for (int i = 0; i < num_prev; i++)
    {
        if (p == prev[i])
            return false;
    }
    return true;
}

uint64_t *random_primes(int prime_count, uint64_t prime_max)
{
    uint64_t *primes = (uint64_t *) malloc(prime_count * sizeof(uint64_t));
    
    for (int i = 0; i < prime_count; i++)
    {
        uint64_t p;
        do
            p = rand() % prime_max;
        while (!is_prime(p) || !is_distinct(p, primes, i));
        
        primes[i] = p;
    }
    
    return primes;
}

void shuffle(uint64_t *array, int size)
{
    for (int i = 0; i < size; i++)
    {
        int j = rand() % (size - i) + i;
        uint64_t tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

void write_numbers(uint64_t *numbers, int num_count, char *path, int file_count)
{
    FILE **files = (FILE **) malloc(file_count * sizeof(FILE *));
    char filename[FILENAME_SIZE], filenum[FILENUM_SIZE];
    
    for (int i = 0; i < file_count; i++)
    {
        sprintf(filenum, "%04d", i);
        strcpy(filename, path);
        strcat(filename, filenum);
        files[i] = fopen(filename, "w");
    }
    
    for (int i = 0; i < num_count; i++)
    {
        int j = i % file_count;
        uint64_t number = htobe64(numbers[i]);
        fwrite(&number, sizeof(uint64_t), 1, files[j]);
    }
    
    if (debug)
    {
        for (int i = 0; i < file_count; i++)
        {
            printf("\nFile %d:\n", i);
            for (int j = i; j < num_count; j += file_count)
                printf("%llu\n", (unsigned long long) numbers[j]);
        }
    }
    
    for (int i = 0; i < file_count; i++)
        fclose(files[i]);
    free(files);
}

void help()
{
    printf("Usage: ./gen PATH [--file-count FILECOUNT] [--num-count NUMCOUNT --prime-count PRIMECOUNT] [--prime-max PRIMEMAX] [--num-max NUMMAX] [--debug]\n");
    printf("Limitations:\n");
    printf("- NUMCOUNT may not be smaller than PRIMECOUNT\n");
    printf("- PRIMECOUNT must be at least 2\n");
    printf("- there must be at least PRIMECOUNT primes smaller than PRIMEMAX\n");
    printf("- NUMMAX must fit in a 64bit unsigned integer\n");
    printf("- the square of PRIMEMAX may not be bigger than NUMMAX\n");
    printf("- FILECOUNT must be smaller than 10000\n");
    printf("- PATH may not be longer than 50 characters\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int file_count = DEFAULT_FILECOUNT,
            num_count = DEFAULT_NUMCOUNT,
            prime_count = DEFAULT_PRIMECOUNT;
    uint64_t prime_max = DEFAULT_PRIMEMAX,
            num_max = DEFAULT_NUMMAX;
    char *path = NULL;
    
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], ARGNAME_FILECOUNT) == 0)
        {
            if (++i == argc)
                help();
            file_count = atoi(argv[i]);
        }
        else if (strcmp(argv[i], ARGNAME_NUMCOUNT) == 0)
        {
            if (++i == argc)
                help();
            num_count = atoi(argv[i]);
        }
        else if (strcmp(argv[i], ARGNAME_PRIMECOUNT) == 0)
        {
            if (++i == argc)
                help();
            prime_count = atoi(argv[i]);
        }
        else if (strcmp(argv[i], ARGNAME_PRIMEMAX) == 0)
        {
            if (++i == argc)
                help();
            prime_max = atoi(argv[i]);
        }
        else if (strcmp(argv[i], ARGNAME_NUMMAX) == 0)
        {
            if (++i == argc)
                help();
            num_max = atoi(argv[i]);
        }
        else if (strcmp(argv[i], ARGNAME_DEBUG) == 0)
        {
            debug = true;
        }
        else
        {
            path = argv[i];
        }
    }
    
    if (path == NULL || strlen(path) > PATH_MAX_LENGTH
            || num_count < prime_count || file_count >= FILECOUNT_MAX)
        help();
    
    gen_small_primes();
    srand(time(NULL));
    uint64_t *primes = random_primes(prime_count, prime_max);
    if (debug)
    {
        printf("Generated primes:\n");
        for (int i = 0; i < prime_count; i++)
            printf("%llu\n", (unsigned long long) primes[i]);
    }
    uint64_t sol_prime = primes[0];
    
    uint64_t *base_factors = (uint64_t *) malloc(2 * num_count * sizeof(uint64_t));
    base_factors[0] = sol_prime;
    for (int i = 1; i < 2 * num_count; i++)
    {
        int j = (i - 1) % (prime_count - 1) + 1;
        base_factors[i] = primes[j];
    }
    shuffle(base_factors, 2 * num_count);
    
    int sol_file;
    for (int i = 0; i < 2 * num_count; i++)
    {
        if (base_factors[i] == sol_prime)
            sol_file = (i / 2) % file_count;
    }
    
    uint64_t *final_numbers = (uint64_t *) malloc(num_count * sizeof(uint64_t));
    for (int i = 0; i < num_count; i++)
    {
        final_numbers[i] = base_factors[2*i] * base_factors[2*i+1];
        while (final_numbers[i] <= num_max / prime_max)
            final_numbers[i] *= primes[rand() % (prime_count - 1) + 1];
    }
    
    write_numbers(final_numbers, num_count, path, file_count);
    
    if (debug)
        printf("\nSolution: prime %llu in file %d.\n", (unsigned long long) sol_prime, sol_file);
    
    free(primes);
    free(base_factors);
    free(final_numbers);
}
