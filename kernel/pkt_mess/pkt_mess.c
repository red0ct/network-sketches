#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <net/tcp.h>

#define DEBUG
#define MARK "[NFH]"
#ifdef DEBUG
#define nfh_dbg(str, ...) printk(KERN_ALERT MARK "(%s): " str "\n", __func__, ##__VA_ARGS__)
#else
#define nfh_dbg(str, ...) no_printk(KERN_ALERT MARK "(%s): " str "\n", __func__, ##__VA_ARGS__)
#endif

static unsigned int nfh_handler(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) /* since 4.4 */
{
    struct iphdr *ip_header = ip_hdr(skb);

    if(ip_header->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_header = tcp_hdr(skb);
        if((unsigned int)ntohs(tcp_header->dest) == 15004) {
            unsigned char *data = (char *)tcp_header + tcp_hdrlen(skb);
            /*
            uint16_t payload = skb->len - ip_hdrlen(skb) - sizeof(struct tcphdr);
            unsigned char *tail_ptr = skb_tail_pointer(skb);
            unsigned char *data = tail_ptr - payload;
            */
            strncpy(data, "XXXXXX", 6);
            tcp_header->check = 0;
            tcp_header->check = tcp_v4_check(ntohs(ip_header->tot_len) - (ip_header->ihl << 2),
                      ip_header->saddr, ip_header->daddr, csum_partial(tcp_header, tcp_header->doff << 2, 0));
            nfh_dbg("Mangled");
        }
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops nfhops = {
    .hook       = nfh_handler,
    .hooknum    = NF_INET_LOCAL_OUT,
    .pf         = PF_INET,
    .priority   = NF_IP_PRI_FIRST,
};

static int __init init_nfh(void)
{
    if(nf_register_hook(&nfhops)) {
        nfh_dbg("Hook register error");
        return -1;
    }
    nfh_dbg("Registered");

    return 0;
}

static void __exit exit_nfh(void)
{
    nf_unregister_hook(&nfhops);
    nfh_dbg("Unregistered");
}

module_init(init_nfh);
module_exit(exit_nfh);
MODULE_LICENSE("GPL");
