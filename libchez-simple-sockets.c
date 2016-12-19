#include <unistd.h>       // for close

#include <sys/types.h>    // for socket, connect, getaddrinfo and accept
#include <sys/socket.h>   // for socket, connect, getaddrinfo, accept and shutdown
#include <netinet/in.h>   // for sockaddr_in
#include <arpa/inet.h>    // for htons and inet_pton
#include <netdb.h>        // for getaddrinfo

#include <string.h>       // for memset and memcpy
#include <stdint.h>       // for uint8_t and uint32_t
#include <errno.h>

// these are in the 'scheme' binary and can be statically linked
// against
int Sactivate_thread(void);
void Sdeactivate_thread(void);
void Slock_object(void*);
void Sunlock_object(void*);

// arguments: if port is greater than 0, it is set as the port to
// which the connection will be made, otherwise this is deduced from
// the service argument.  The service argument may be NULL, in which
// case a port number greater than 0 must be given.

// return value: file descriptor of socket, or -1 on failure to look
// up address, -2 on failure to construct a socket, -3 on a failure to
// connect
int connect_to_ipv4_host(const char* address, const char* service, unsigned short port) {

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // getaddrinfo and connect may show latency - release the GC
  Slock_object((void*)address);
  if (service) Slock_object((void*)service);
  Sdeactivate_thread();

  struct addrinfo* info;
  if (getaddrinfo(address, service, &hints, &info)
      || info == NULL) {
    Sactivate_thread();
    Sunlock_object((void*)address);
    if (service) Sunlock_object((void*)service);
    return -1;
  }

  int sock;
  struct addrinfo* tmp;
  int err = 0;

  // loop through the offered numeric addresses
  for (tmp = info; tmp != NULL; tmp = tmp->ai_next) {
    err = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      err = -2;
      continue;
    }
    
    struct sockaddr* in = tmp->ai_addr;
    // if we passed NULL to the service argument of getaddrinfo, we
    // have to set the port number by hand or connect will fail
    if (port)
      ((struct sockaddr_in*)in)->sin_port = htons(port);
    int res;
    do {
      res = connect(sock, in, sizeof(struct sockaddr_in));
    } while (res == -1 && errno == EINTR);
    if (res) {
      close(sock);
      err = -3;
      continue;
    }
    else break;
  }

  Sactivate_thread();
  Sunlock_object((void*)address);
  if (service) Sunlock_object((void*)service);
  freeaddrinfo(info);

  if (err) return err;
  return sock;
}

// arguments: if port is greater than 0, it is set as the port to
// which the connection will be made, otherwise this is deduced from
// the service argument.  The service argument may be NULL, in which
// case a port number greater than 0 must be given.

// return value: file descriptor of socket, or -1 on failure to look
// up address, -2 on failure to construct a socket, -3 on a failure to
// connect
int connect_to_ipv6_host(const char* address, const char* service, unsigned short port) {

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;

  // getaddrinfo and connect may show latency - release the GC
  Slock_object((void*)address);
  if (service) Slock_object((void*)service);
  Sdeactivate_thread();

  struct addrinfo* info;
  if (getaddrinfo(address, service, &hints, &info)
      || info == NULL) {
    Sactivate_thread();
    Sunlock_object((void*)address);
    if (service) Sunlock_object((void*)service);
    return -1;
  }

  int sock;
  struct addrinfo* tmp;
  int err = 0;

  // loop through the offered numeric addresses
  for (tmp = info; tmp != NULL; tmp = tmp->ai_next) {
    err = 0;
    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
      err = -2;
      continue;
    }
    
    struct sockaddr* in = tmp->ai_addr;
    // if we passed NULL to the service argument of getaddrinfo, we
    // have to set the port number by hand or connect will fail
    if (port)
      ((struct sockaddr_in6*)in)->sin6_port = htons(port);
    int res;
    do {
      res = connect(sock, in, sizeof(struct sockaddr_in6));
    } while (res == -1 && errno == EINTR);
    if (res) {
      close(sock);
      err = -3;
      continue;
    }
    else break;
  }

  Sactivate_thread();
  Sunlock_object((void*)address);
  if (service) Sunlock_object((void*)service);
  freeaddrinfo(info);

  if (err) return err;
  return sock;
}

// arguments: if local is true, the socket will only listen on
// localhost.  If false, it will listen for any address.  port is the
// port to listen on.  backlog is the maximum number of queueing
// connections.

// return value: file descriptor of socket, or -1 on failure to make
// an address, -2 on failure to create a socket, -3 on a failure to
// bind to the socket, and -4 on a failure to listen on the socket
int listen_on_ipv4_socket(int local, unsigned short port, int backlog) {

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  if (local) {
    if ((inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr))) == -1)
      return -1;
  }
  else
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return -2;

  addr.sin_port = htons(port);
    
  if ((bind(sock, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
    close(sock);
    return -3;
  }

  if ((listen(sock, backlog)) == -1) {
    close(sock);
    return -4;
  }

  return sock;
}

// arguments: if local is true, the socket will only listen on
// localhost.  If false, it will listen for any address.  port is the
// port to listen on.  backlog is the maximum number of queueing
// connections.

// return value: file descriptor of socket, or -1 on failure to make
// an address, -2 on failure to create a socket, -3 on a failure to
// bind to the socket, and -4 on a failure to listen on the socket
int listen_on_ipv6_socket(int local, unsigned short port, int backlog) {

  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));

  addr.sin6_family = AF_INET6;
  if (local) {
    if ((inet_pton(AF_INET6, "::1", &(addr.sin6_addr))) == -1)
      return -1;
  }
  else
    addr.sin6_addr = in6addr_any;

  int sock = socket(AF_INET6, SOCK_STREAM, 0);
  if (sock == -1)
    return -2;

  addr.sin6_port = htons(port);
    
  if ((bind(sock, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
    close(sock);
    return -3;
  }

  if ((listen(sock, backlog)) == -1) {
    close(sock);
    return -4;
  }

  return sock;
}

// arguments: sock is the file descriptor of the socket on which to
// accept connections, as returned by listen_on_ipv4_socket.
// connection is an array of size 4 in which the binary address of the
// connecting client will be placed

// return value: file descriptor for the connection on success, -1 on
// failure
int accept_ipv4_connection(int sock, uint32_t* connection) {

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t addr_len = sizeof(addr);

  // release the GC for accept() call
  Slock_object((void*)connection);
  Sdeactivate_thread();

  int connect_sock;
  do {
    connect_sock = accept(sock, (struct sockaddr*)&addr, &addr_len);
  } while (connect_sock == -1 && errno == EINTR);

  Sactivate_thread();
  Sunlock_object((void*)connection);

  if (addr_len > sizeof(addr)) {
    close(connect_sock);
    return -1;
  }
  if (connect_sock == -1) return -1;
  memcpy(connection, &addr.sin_addr.s_addr, sizeof(uint32_t));
  return connect_sock;
}

// arguments: sock is the file descriptor of the socket on which to
// accept connections, as returned by listen_on_ipv6_socket.
// connection is an array of size 16 in which the binary address of
// the connecting client will be placed

// return value: file descriptor for the connection on success, -1 on
// failure
int accept_ipv6_connection(int sock, uint8_t* connection) {

  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t addr_len = sizeof(addr);

  // release the GC for accept() call
  Slock_object((void*)connection);
  Sdeactivate_thread();

  int connect_sock;
  do {
    connect_sock = accept(sock, (struct sockaddr*)&addr, &addr_len);
  } while (connect_sock == -1 && errno == EINTR);

  Sactivate_thread();
  Sunlock_object((void*)connection);

  if (addr_len > sizeof(addr)) {
    close(connect_sock);
    return -1;
  }
  if (connect_sock == -1) return -1;
  memcpy(connection, &addr.sin6_addr.s6_addr, sizeof(addr.sin6_addr.s6_addr));
  return connect_sock;
}

int shutdown_(int fd, int how) {
  switch (how) {
  case 0:
    return (shutdown(fd, SHUT_RD) == 0);
  case 1:
    return (shutdown(fd, SHUT_WR) == 0);
  case 2:
    return (shutdown(fd, SHUT_RDWR) == 0);
  default:
    return 0;
  }
}

int close_fd(int fd) {
  return (close(fd) == 0);
}

// just hand of to Unix read, to avoid having to load libc in
// sockets.ss
int c_read(int fd, void* buf, size_t n) {
  return read(fd, buf, n);
}

// just hand of to Unix write, to avoid having to load libc in
// sockets.ss
int c_write(int fd, const void* buf, size_t n) {
  return write(fd, buf, n);
}
