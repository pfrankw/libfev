#include <stdio.h>

#include <fev/fev.h>
#include <fev/time.h>

void timer(void *arg)
{
    printf("TIMER!\n");
}

int main()
{

    fev_t fev;

    fev_init(&fev);

    fev_add_timer(&fev, 1000, 0, timer, 0);

    while (1) {
        fev_run(&fev);
        fev_sleep_usec(50*1000);
    }


    fev_free(&fev);
}
