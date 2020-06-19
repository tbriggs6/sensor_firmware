#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_client(int c)
{
    char buf[8192];
    char *lastpos;
    int size;

    while (1) {
        size = recv(c, buf, 8192, 0);
        if (size == 0) {
            break;
        }
        for (int i = 0; i < size; i++) {
          printf("%-2.2x ", buf[i]);
        }
        printf("\n");

        buf[0] = 'O';  buf[1] = 'K'; buf[2] = 'S'; buf[3] = 'V'; buf[4] = 'R';

        send(c, buf, 5, 0);
    }
}

int main()
{
    int s, c;
    int reuseaddr = 1;
    struct sockaddr_in6 addr;
    int pid;

    printf("Listening for incoming on 5555\n");
    s = socket(AF_INET6, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(5555);
    addr.sin6_addr = in6addr_any;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 5);

    while (1) {
        c = accept(s, NULL, NULL);
        pid = fork();
        if (pid == -1) {
            exit(1);
        } else if (pid == 0) {
            close(s);
            handle_client(c);
            close(c);
            return 0;
        } else {
            close(c);
            waitpid(pid, NULL, 0);
        }
    }
}
