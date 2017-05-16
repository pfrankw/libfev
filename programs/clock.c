#include <inttypes.h>
#include <stdio.h>

#include <fev/time.h>

int main(){

    uint64_t c = fev_clock();
    printf("%"PRIu64"\n", c);
}
