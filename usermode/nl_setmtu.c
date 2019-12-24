/*
 * Just as example of setting MTU on particular
 * interface through the Netlink API.
 * Run binary with 'sudo'.
 */
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

#define IFACE_NAME "enp9s0" /* f.e. */

int main(void) {
     int ret, nl_sock;
     unsigned int mtu = 8000;
     struct rtattr  *rta;
     struct {
          struct nlmsghdr nh;
          struct ifinfomsg  ifinfo;
          char   attrbuf[512];
     } req = {0};

     nl_sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
     if(nl_sock < 0) {
          perror("socket():");
          return -1;
     }

     req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
     req.nh.nlmsg_flags = NLM_F_REQUEST;
     req.nh.nlmsg_type = RTM_NEWLINK; /* RTM_SETLINK */
     req.ifinfo.ifi_family = AF_UNSPEC;
     req.ifinfo.ifi_index = if_nametoindex(IFACE_NAME);
     if(!req.ifinfo.ifi_index) {
          perror("if_nametoindex():");
          return -1;
     }
     printf("%s index: %d\n", IFACE_NAME, req.ifinfo.ifi_index);
     req.ifinfo.ifi_change = 0xFFFFFFFF;
     rta = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.nh.nlmsg_len));
     rta->rta_type = IFLA_MTU;
     rta->rta_len = RTA_LENGTH(sizeof(unsigned int));
     req.nh.nlmsg_len = NLMSG_ALIGN(req.nh.nlmsg_len) + RTA_LENGTH(sizeof mtu);
     memcpy(RTA_DATA(rta), &mtu, sizeof mtu);

     ret = send(nl_sock, &req, req.nh.nlmsg_len, 0);
     if(ret < 0) {
          perror("send():");
          return -1;
     }

     return 0;
}
