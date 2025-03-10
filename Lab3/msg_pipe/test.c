#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
from Linux Source code
*/

int
main(int argc, char *argv[])
{
    int    pipefd[2];
    char   buf;
    pid_t  cpid;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1)
        err(EXIT_FAILURE, "pipe");

    cpid = fork();
    if (cpid == -1)
        err(EXIT_FAILURE, "fork");

    if (cpid == 0) {    /* Child reads from pipe */
        if (close(pipefd[1]) == -1)  /* Close unused write end */
            err(EXIT_FAILURE, "close");

        while (read(pipefd[0], &buf, 1) > 0) {
            if (write(STDOUT_FILENO, &buf, 1) != 1)
                err(EXIT_FAILURE, "write");
        }

        if (write(STDOUT_FILENO, "\n", 1) != 1)
            err(EXIT_FAILURE, "write");
        if (close(pipefd[0]) == -1)
            err(EXIT_FAILURE, "close");
        _exit(EXIT_SUCCESS);

    } else {            /* Parent writes argv[1] to pipe */
        if (close(pipefd[0]) == -1)  /* Close unused read end */
            err(EXIT_FAILURE, "close");
        if (write(pipefd[1], argv[1], strlen(argv[1])) != strlen(argv[1]))
            err(EXIT_FAILURE, "write");
        if (close(pipefd[1]) == -1)  /* Reader will see EOF */
            err(EXIT_FAILURE, "close");
        if (wait(NULL) == -1)        /* Wait for child */
            err(EXIT_FAILURE, "wait");
        exit(EXIT_SUCCESS);
    }
}