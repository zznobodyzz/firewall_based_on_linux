#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/raw.h>
#include <net/checksum.h>
#include <net/inet_ecn.h>
#include <linux/netfilter_ipv4.h>
#include <net/xfrm.h>
#include <linux/mroute.h>
#include <linux/netlink.h>
#define MAGIC_NUM 'Z'
#define ADD_IP _IOW(MAGIC_NUM,0,int)
#define ADD_SRC_PORT _IOW(MAGIC_NUM,1,int)
#define ADD_DST_PORT _IOW(MAGIC_NUM,2,int)
#define DEL_IP _IOW(MAGIC_NUM,3,int)
#define DEL_SRC_PORT _IOW(MAGIC_NUM,4,int)
#define DEL_DST_PORT _IOW(MAGIC_NUM,5,int)
spinlock_t lock;
__be32 keep_out_ip[64]={0};
__be16 keep_out_source_port[64]={0};
__be16 keep_out_dest_port[64]={0};
dev_t devno;
int major=0;
int minor=0;
int port_flag=0;
int section_flag=0;
struct class * clsp;
struct device * devp;
void print_addr(__u32 saddr)
{
	unsigned char * p=&saddr;
	printk("%d.%d.%d.%d\n",p[0],p[1],p[2],p[3]);
}
int simple_check(struct iphdr * iph,struct sk_buff *skb,int vector)
{
	int i;
	unsigned short port;
	struct tcphdr * tcph;
	struct udphdr * udph;
	tcph=tcp_hdr(skb);
	udph=udp_hdr(skb);
	//printk("udp--->sport=%d,dport=%d\n",ntohs(udph->source),ntohs(udph->dest));
	spin_lock(&lock);
	for(i=0;i<64;i++)
	{
		if(iph->saddr==keep_out_ip[i])
		{
			spin_unlock(&lock);
			printk("DROP package send to/receive from ");
			print_addr(iph->saddr);
			return 1;
		}
	}
	if(vector==0)
	{
		if(iph->protocol==17)
			port=udph->source;
		else if(iph->protocol==6)
			port=tcph->source;
	}
	else if(vector==1)
	{
		if(iph->protocol==17)
			port=udph->dest;
		else if(iph->protocol==6)
			port=tcph->dest;
	}
	if(vector==0)
	{
		for(i=0;i<64;i++)
		{
			if(port==keep_out_source_port[i])
			{
				spin_unlock(&lock);
				printk("DROP package receive from port %hu\n",ntohs(port));
				return 1;
			}
		}
	}
	else if(vector==1)
	{
		for(i=0;i<64;i++)
		{
			if(port==keep_out_dest_port[i])
			{
				spin_unlock(&lock);
				printk("DROP package send to port %hu\n",ntohs(port));
				return 1;
			}
		}
	}
	spin_unlock(&lock);
	return 0;
}
unsigned int packet_filter_pre(unsigned int hooknum,struct sk_buff *skb,const struct net_device *in,const struct net_device *out,int (*okfn)(struct sk_buff *))
{
	//printk("hooknum:%d   in:%s   out:%s \n",hooknum,in->name,out->name);
	struct iphdr * iph;
	struct ethhdr * ethh;
	if(!(ethh=eth_hdr(skb)))
	return NF_ACCEPT;
	iph=ip_hdr(skb);
	if(iph->ihl<5||iph->version!=4)
		return NF_ACCEPT;
	print_addr(iph);
	if(simple_check(iph,skb,0))
		return NF_DROP;
	return NF_ACCEPT;
}
unsigned int packet_filter_post(unsigned int hooknum,struct sk_buff *skb,const struct net_device *in,const struct net_device *out,int (*okfn)(struct sk_buff *))
{
	//printk("hooknum:%d   in:%s   out:%s \n",hooknum,in->name,out->name);
	struct iphdr * iph;
	struct ethhdr * ethh;
	if(!(ethh=eth_hdr(skb)))
		return NF_ACCEPT;
	iph=ip_hdr(skb);
	if(iph->ihl<5||iph->version!=4)
		return NF_ACCEPT;
	print_addr(iph);
	if(simple_check(iph,skb,1))
		return NF_DROP;
	return NF_ACCEPT;
}
static struct nf_hook_ops packet_filter_opt[] = {
	{
		.hook                = packet_filter_pre,
		.owner                = THIS_MODULE,
		.pf                = PF_INET,
		.hooknum        = NF_INET_PRE_ROUTING,
		.priority          = NF_IP_PRI_FIRST,
	},
	{
		.hook                = packet_filter_post,
		.owner                = THIS_MODULE,
		.pf                = PF_INET,
		.hooknum        = NF_INET_POST_ROUTING,
		.priority          = NF_IP_PRI_FIRST,
	},		
};
int my_add_ip(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_ip[i]==data)
		{
			spin_unlock(&lock);
			return -ENOMEM;
		}
	}
	for(i=0;i<64;i++)
	{
		if(keep_out_ip[i]==0)
		{
			keep_out_ip[i]=data;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
int my_add_source_port(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_source_port[i]==data)
		{
			spin_unlock(&lock);
			return -ENOMEM;
		}
	}
	for(i=0;i<64;i++)
	{
		if(keep_out_source_port[i]==0)
		{
			keep_out_source_port[i]=data;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
int my_add_dest_port(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_dest_port[i]==data)
		{	
			spin_unlock(&lock);
			return -ENOMEM;
		}
	}
	for(i=0;i<64;i++)
	{
		if(keep_out_dest_port[i]==0)
		{
			keep_out_dest_port[i]=data;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
int my_del_ip(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_ip[i]==data)
		{
			keep_out_ip[i]=0;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
int my_del_source_port(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_source_port[i]==data)
		{
			keep_out_source_port[i]=0;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
int my_del_dest_port(unsigned long data)
{
	int i;
	for(i=0;i<64;i++)
	{
		if(keep_out_dest_port[i]==data)
		{
			keep_out_dest_port[i]=0;
			spin_unlock(&lock);
			return 0;
		}
	}
	spin_unlock(&lock);
	return -ENOMEM;
}
long filter_ioctl(struct file * file, unsigned int cmd, unsigned long data)
{
	if(_IOC_TYPE(cmd)!='Z')
		return 0;
	spin_lock(&lock);
	switch(cmd)
	{
		case ADD_IP:
			return my_add_ip(data);
		case ADD_SRC_PORT:
			return my_add_source_port(data);
		case ADD_DST_PORT:
			return my_add_dest_port(data);
		case DEL_IP:
			return my_del_ip(data);
		case DEL_SRC_PORT:
			return my_del_source_port(data);
		case DEL_DST_PORT:
			return my_del_dest_port(data);
		default:
			return -EAGAIN;
	}
}
int filter_open(struct inode  * inode,struct file * file)
{
		printk("filter is now open\n");
		return 0;
}
int filter_release(struct inode * inode,struct file * file)
{
		printk("filter is now close\n");
		return 0;
}
struct file_operations filter_fops={
	.unlocked_ioctl=filter_ioctl,
	.open=filter_open,
	.release=filter_release,
};
static int __init filter_init(void)
{
	major=register_chrdev(major ,"net_fire_wall", &filter_fops);
	printk("dev major:%d\n",major);
	devno=MKDEV(major,minor);
	clsp=class_create(THIS_MODULE,"net_fire_wall");
	devp=device_create(clsp,NULL,devno,NULL,"net_fire_wall");
	nf_register_hooks(packet_filter_opt, ARRAY_SIZE(packet_filter_opt));
	spin_lock_init(&lock);
	printk("net fire wall init complete\n");
	return 0;
}
static void __exit filter_exit(void)
{
	nf_unregister_hooks(packet_filter_opt,ARRAY_SIZE(packet_filter_opt));
	unregister_chrdev(major,"net_fire_wall");
	device_destroy(clsp,devno);
	class_destroy(clsp);
	printk("net fire wall exit complete\n");
}
MODULE_LICENSE("GPL");
module_init(filter_init);
module_exit(filter_exit);
