#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include <fev/fev.h>
#include <fev/time.h>

static int accept_fd( int *bind_fd,
                        int *client_fd,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
    int ret;
    int type;

    struct sockaddr_storage client_addr;

#if defined(__socklen_t_defined) || defined(_SOCKLEN_T) ||  \
    defined(_SOCKLEN_T_DECLARED) || defined(__DEFINED_socklen_t)
    socklen_t n = (socklen_t) sizeof( client_addr );
    socklen_t type_len = (socklen_t) sizeof( type );
#else
    int n = (int) sizeof( client_addr );
    int type_len = (int) sizeof( type );
#endif

    ret = (*client_fd = (int) accept( *bind_fd, (struct sockaddr *) &client_addr, &n ));

    if( ret < 0 )
        return -1;

    if( client_ip != NULL )
    {
        if( client_addr.ss_family == AF_INET )
        {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) &client_addr;
            *ip_len = sizeof( addr4->sin_addr.s_addr );

            if( buf_size < *ip_len )
                return -1;

            memcpy( client_ip, &addr4->sin_addr.s_addr, *ip_len );
        }
        else
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &client_addr;
            *ip_len = sizeof( addr6->sin6_addr.s6_addr );

            if( buf_size < *ip_len )
                return -1;

            memcpy( client_ip, &addr6->sin6_addr.s6_addr, *ip_len);
        }
    }

    return 0;
}

static int bind_fd(int *fd, const char *bind_ip, const char *port)
{

    int n, r = -1;
    struct addrinfo hints, *addr_list = 0, *cur = 0;

    memset( &hints, 0, sizeof( hints ) );
    *fd = -1;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if( !bind_ip )
        hints.ai_flags = AI_PASSIVE;

    if( getaddrinfo( bind_ip, port, &hints, &addr_list ) != 0 )
        goto cleanup;

    for( cur = addr_list; cur != 0; cur = cur->ai_next )
    {
        *fd = (int) socket( cur->ai_family, cur->ai_socktype,
                            cur->ai_protocol );
        if( *fd < 0 )
            goto cleanup;

        n = 1;
        if( setsockopt( *fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &n, sizeof( n ) ) != 0 )
            goto cleanup;

        if( bind( *fd, cur->ai_addr, (int) cur->ai_addrlen ) != 0 )
            goto cleanup;

        if( listen( *fd, 100 ) != 0 )
            goto cleanup;

        r = 0;
        break;
    }

cleanup:

    if (addr_list)
        freeaddrinfo(addr_list);

    if (r != 0 && *fd != -1)
        close(*fd);

    return r;
}

static void on_client_data(struct fev *fev, struct fev_io *io, uint8_t rev, void *arg)
{
    char buf[4096];
    char *http_res = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";


    read(io->fd, buf, sizeof(buf));
    write(io->fd, http_res, strlen(http_res));
    write(io->fd, "HELLO!", 6);

    shutdown(io->fd, SHUT_RDWR);
    close(io->fd);
    fev_del_io(fev, io);
}

static void on_client(struct fev *fev, struct fev_io *io, uint8_t rev, void *arg)
{


    int client_fd;

    if (accept_fd(&io->fd, &client_fd, 0, 0, 0) != 0)
        goto cleanup;

    fcntl( client_fd, F_SETFL, fcntl( client_fd, F_GETFL ) | O_NONBLOCK );

    //printf("CLIENT!\n");
    fev_add_io(fev, client_fd, FEV_EVENT_READ, on_client_data, 0);

cleanup:
    return;
}

int main()
{
    int r = -1;
    int server_fd = -1;
    fev_t fev;
    struct fev_io *io;

    signal(SIGPIPE, SIG_IGN);

    fev_init(&fev);

    if (bind_fd(&server_fd, 0, "8080") != 0)
        goto cleanup;

    fcntl( server_fd, F_SETFL, fcntl( server_fd, F_GETFL ) | O_NONBLOCK );

    io = fev_add_io(&fev, server_fd, FEV_EVENT_READ, on_client, 0);


    while (1) {
        fev_run(&fev);
    }

    r = 0;
cleanup:
    close(server_fd);
    fev_free(&fev);
    return r;
}
