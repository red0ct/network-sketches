/*
 * Simple polling server.
 */
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_POLL 32
#define LISTEN_PORT 31337
#define LISTEN_BACKLOG 10

void serv(void);

int main(void) {
     serv();

     return 0;
}

void serv(void) {
     int sockfd, acceptfd, i, poll_ret, opt_val = 1;
     struct sockaddr_in sockaddr = {0};
     struct pollfd poll_fd_arr[MAX_POLL];

     if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
          perror("socket():");
          return;
     }
     if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int)) < 0) {
          perror("setsockopt():");
          return;
     }
     sockaddr.sin_family = AF_INET;
     sockaddr.sin_port = htons(LISTEN_PORT);
     if((sockaddr.sin_addr.s_addr = inet_addr("0.0.0.0")) < 0) {
          perror("inet_addr():");
          return;
     }
     if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in))) {
          perror("bind():");
          return;
     }
     if(listen(sockfd, LISTEN_BACKLOG)) {
          perror("listen():");
          return;
     }
     memset(poll_fd_arr, 0, sizeof poll_fd_arr);
     poll_fd_arr[0].fd = sockfd;
     poll_fd_arr[0].events = POLLIN;
     for(i = 1; i < MAX_POLL; ++i)
          poll_fd_arr[i].fd = -1;
     for(;;) {
          int i, recv_len, allow_flag = 0;

          if((poll_ret = poll(&poll_fd_arr[0], MAX_POLL, -1)) == -1) {
               perror("poll():");
               return;
          } else if(! poll_ret) {
               puts("Time is out");
               continue;
          }
          else {
               int counter = 0;
               if(poll_fd_arr[0].revents & POLLIN) { /* adding a new socket */
                    ++counter;
                    acceptfd = accept(sockfd, (struct sockaddr *)NULL, NULL);
                    for(i = 0; i < MAX_POLL; ++i) { /* adding accepted fd */
                         if(poll_fd_arr[i].fd == -1) {
                              poll_fd_arr[i].fd = acceptfd;

                              /* When peer close connection properly - poll() detects it as POLLIN (in most Unix-like).
                                 So the '| POLLRDHUP' isn't needed here */
                              poll_fd_arr[i].events = POLLIN;
                              ++allow_flag;
                              break;
                         }
                    }
                    if(!allow_flag) /* there are no empty cells */
                         if(close(acceptfd))
                              perror("close():");
                    continue;
               }
               for(i = 1; i < MAX_POLL; ++i) {
                    if(poll_ret == counter) /* check if all revents-full cells are visited */
                         break;
                    if(poll_fd_arr[i].fd == -1)
                         continue;
                    if(poll_fd_arr[i].revents != 0) /* check if some revents occured */
                         ++counter;

                    if(poll_fd_arr[i].revents & POLLIN) {
                         /* some trigger, e.g. ---> */
                         char tmp_buff[128] = {0}, str[] = "Go away.\n";
                         if((recv_len = recv(poll_fd_arr[i].fd, tmp_buff, sizeof(tmp_buff) - 1, 0)) < 0) {
                              perror("recv():");
                              return;
                         } else if(!recv_len) {
                              printf("%d closed\n", poll_fd_arr[i].fd);
                              if(close(poll_fd_arr[i].fd))
                                   perror("close():");
                              poll_fd_arr[i].fd = -1;
                              continue;
                         }
                         printf("%s\t[%d bytes recved]\n", tmp_buff, recv_len);
                         if(send(poll_fd_arr[i].fd, str, sizeof(str) - 1, 0) == -1) {
                              perror("send():");
                              return;
                         }
                         /* <--- */
                    }
               }
          }
     }
}
