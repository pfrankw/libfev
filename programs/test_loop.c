#include <stdio.h>
#include <signal.h>

#include <fev/fev.h>
#include <fev/time.h>

fev_t fev_g;
static void sig_handler(int sig)
{
    if (sig == SIGINT) {
        fev_free(&fev_g);
        exit(0);
    }
}

void timer(void *arg)
{
    printf("TIMER!\n");
}

void timer2(void *arg)
{
    printf("TIMER2!\n");
    fev_free(&fev_g);
    exit(0);
}


int main()
{

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        return 0;

    fev_init(&fev_g);


    fev_add_timer(&fev_g, 1000, 0, timer, 0);
    fev_add_timer(&fev_g, 5000, 0, timer2, 0);

    while (1) {
        fev_run(&fev_g);
    }


    fev_free(&fev_g);
}
