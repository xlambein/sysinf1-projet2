#include <endian.h>
#include <inttypes.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    uint64_t number;
    while (scanf("%" SCNu64, &number) == 1)
    {
        number = htobe64(number);
        fwrite(&number, sizeof(uint64_t), 1, stdout);
    }
}
