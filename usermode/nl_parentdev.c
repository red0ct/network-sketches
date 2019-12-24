/*
 * Just as example of obtaining the "parent" device
 * of a given interface through the Netlink API.
 */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <stdio.h>

struct route_req {
     struct nlmsghdr header;
     struct ifinfomsg msg;
};

int list_interfaces() {
     char buf[16192] = {0};
     size_t seq_num = 0;
     struct sockaddr_nl sa = {0};
     struct iovec iov = {0};
     struct msghdr msg = {0};
     struct nlmsghdr *nh;
     struct route_req req = {0};

     int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
     if(fd < 0) {
          perror("socket():");
          return -1;
     }

     req.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
     req.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
     req.header.nlmsg_type = RTM_GETLINK;
     req.header.nlmsg_seq = ++seq_num;
     req.msg.ifi_family = AF_UNSPEC;
     req.msg.ifi_change = 0xFFFFFFFF;

     sa.nl_family = AF_NETLINK;
     iov.iov_base = &req;
     iov.iov_len = req.header.nlmsg_len;
     msg.msg_name = &sa;
     msg.msg_namelen = sizeof sa;
     msg.msg_iov = &iov;
     msg.msg_iovlen = 1;

     if(sendmsg(fd, &msg, 0) < 0) {
          perror("sendmsg():");
          return -1;
     }

     iov.iov_base = buf;
     iov.iov_len = sizeof buf;

     for(;;) {
          int len = recvmsg(fd, &msg, 0);
          if (len < 0) {
               perror("recvmsg():");
               return -1;
          }

          for(nh = (struct nlmsghdr *)buf;
                    NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len)) {
               int len;
               struct ifinfomsg *msg;
               struct rtattr *rta;
               if(nh->nlmsg_type == NLMSG_DONE)
                    return -1;

               if(nh->nlmsg_type != RTM_BASE)
                    continue;

               msg = (struct ifinfomsg *)NLMSG_DATA(nh); /* message payload */
               if(msg->ifi_type != ARPHRD_ETHER)
                    continue;

               printf("\nINDEX: %d\n", msg->ifi_index);

               rta = IFLA_RTA(msg); /* message attributes */
               len = nh->nlmsg_len - NLMSG_LENGTH(sizeof *msg);

               for(; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
                    if(rta->rta_type == IFLA_ADDRESS) {
                         char mac[18] = {0};
                         unsigned char *ch = (unsigned char *)RTA_DATA(rta);

                         snprintf(mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
                                   ch[0], ch[1], ch[2], ch[3], ch[4], ch[5]);

                         printf("MAC: %s\n", mac);
                    }

                    if(rta->rta_type == IFLA_IFNAME)
                         printf("NAME: %s\n", (char *)RTA_DATA(rta));

                    if(rta->rta_type == IFLA_LINK) /* there is a "parent" device */
                         printf("PARENT's INDEX: %u\n", *(unsigned short *)((char *) rta + NLA_HDRLEN));
               }
          }
     }
}

int main(void) {
     list_interfaces();

     return 0;
}
