#include <signal.h>
#include <stdlib.h>

sig_func signal_funcs[7];

int raise(int sig) {
    sig_func signal_func;

    if (sig < 1 || sig > 7) {
        return -1;
    }

    signal_func = signal_funcs[sig - 1];

    if (signal_func != (sig_func)1) {
        signal_funcs[sig - 1] = (sig_func)0;
    }

    if (signal_func == (sig_func)1 || (signal_func == (sig_func)0 && sig == 1)) {
        return 0;
    }

    if (signal_func == (sig_func)0) {
        exit(0);
    }

    (*signal_func)(sig);
    return 0;
}
