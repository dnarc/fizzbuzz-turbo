/* Twist on the usual fizzbuzz program:
 *
 * Calculates and prints fizzbuzz but also uses poll() and signalfd() 
 * to wait for SIGUSR1 signal to print out number of fizzes.
 * 
 * Compile: gcc -std=c99 -D_POSIX_C_SOURCE=1 -o fbt fbt.c
 *
 * Usage: kill -USR1 <fbt pid>
*/


#include <stdio.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <poll.h>

#define FB_CONST_1  3
#define FB_CONST_2  5
#define MAX_EVENTS  1 


/* Received SIGUSR1, print out the number of fizzes. */
void handle_fizz_request(int sigfd, unsigned n_fizz)
{
    /* Clear the signal fd data. */
    
    struct signalfd_siginfo info;
    read(sigfd, &info, sizeof(info));    
    printf("got %u fizzes\n", n_fizz); 
}

int main(int argc, char** argv)
{
    unsigned num_fizz = 0, num_buzz = 0;

    for (unsigned i = 1; i <= 100; i++)
    {
        printf("%u: ", i);
        if ((i % FB_CONST_1) == 0)
        {
            printf("fizz\t");
            num_fizz++;
        }
        if ((i % FB_CONST_2) == 0)
        {
            printf("buzz");
            num_buzz++;
        }    
        printf("\n");
    }
  
    /* Event-listening block. */
    sigset_t sig_set;
    int fd_sigfd, nfds;
    struct pollfd events[MAX_EVENTS];
    struct pollfd poll_sig;

    /* Get signal fd for SIGUSR1. */
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig_set, NULL);
    fd_sigfd = signalfd(-1, &sig_set, SFD_NONBLOCK | SFD_CLOEXEC);
    if (-1 == fd_sigfd)
    {
        perror("signalfd\n");
        _exit(1);
    }
    poll_sig.fd = fd_sigfd;
    poll_sig.events = POLLIN | POLLHUP | POLLERR;
    events[0] = poll_sig;
    
    for (;;)
    {
        nfds = poll(events, MAX_EVENTS, -1);
        if (-1 == nfds)
        {
            perror("poll\n");
            _exit(1);
        }
        /* Print out number of fizzes on receipt of signal. */
        if (POLLIN & events[0].revents)
        {
            handle_fizz_request(fd_sigfd, num_fizz);
        }
        else
        {
            perror("poll\n");
            _exit(1);
        }
    }

}
