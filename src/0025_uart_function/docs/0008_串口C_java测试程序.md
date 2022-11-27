# 串口C java测试程序

整理一下串口测试程序。

# 参考

* [Android app] Linux串口驱动配置，可执行程序测试，App串口通信程序](https://blog.csdn.net/John_chaos/article/details/121203488)

# 串口ttyHSL1的C测试程序 （ndk编译，可执行文件推到system/bin/测试）

```C++
#include <stdio.h>     
#include <stdlib.h>     
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>  
#include <fcntl.h>    
#include <errno.h>      
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
 
#define msleep(x)	usleep(x * 1000)
 
 
/*
*	这里类似波特率，需要可以加上多个串口节点
*	
*/
int select_serial_node(int num, char* name)
{
	switch(num)
	{
		case 1:
			strcpy(name, "/dev/ttyHSL1");
			break;
		case 2:
			strcpy(name, "/dev/ttyHSL2");
			break;
		default:
			return -1;
	}
	
	return 0;
}
 
/*
*		
*   设置波特率，这里case没多写，就写 2 个常用的9600和 115200
*/
unsigned int set_baud_rate(int br)
{
	unsigned int baud;
	switch (br)
	{
		case 0:
			baud = B0;
			break;
		case 9600:
			baud = B9600;
			break;
		case 115200:
			baud = B115200;
			break;
		default:
			printf("input err:baud rate not support");
			return -1;
	}
	
	return baud;
}
 
int init_serial_device(char *name, int baud)
{
	int fd;
	int ret;
	struct termios options;
	
	fd = open(name, O_RDWR | O_NDELAY | O_NOCTTY);
	
	if(fd == -1) 
	{
		printf("%s: open error\n", name);
		return -1;
	}
	
	//函数tcgetattr，用于获取终端参数，到options变量
	ret = tcgetattr(fd, &options);
	if (-1 == ret)
		return -1;
	
	options.c_cflag &= ~CSIZE;		//屏蔽其他标志
	options.c_cflag |= CS8;			//数据8bit
	options.c_cflag &= ~PARENB;		//无校验
	options.c_cflag &= ~CSTOPB;		//设置1位停止位
	
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |IXON);
	ret = tcsetattr(fd, TCSANOW, &options);
	if (-1 == ret)
		return -1;
	return fd;
}
 
void send_serial_data(int fd, char *ptr)
{
	int ret;
	ret = write(fd, ptr, strlen(ptr));
	msleep(10);
}
 
/*
*	ex:  ./ttyTest 1 115200 "hello JnhnChaos!"
*
*/
int main(int argc, char **argv)
{
	int fd;
	int bn;
	int ret;
	char serial_node_name[25];
	int serial_num;
	char message_s[128];
	
	speed_t baud;
	
	printf("qyc at main begin\n");
 
	if (argc != 4) 
	{
		printf("input error: args should be set to 4 !\n");
		exit(-1);
	}
	
	//ret = sscanf(argv[1], "%d", &serial_num);
	serial_num = atoi(argv[1]);
	memset(serial_node_name, 0, sizeof(serial_node_name));
	ret = select_serial_node(serial_num, serial_node_name);
	if(ret == -1) return -1;
	printf("qyc, serial_node_name == %s\n", serial_node_name);
	
 
	//ret = sscanf(argv[2], "%d", &bn);
	bn = atoi(argv[2]);
	baud = set_baud_rate(bn);
	if (ret == -1) return -1;
	
	
	if(strlen(argv[3]) > 128)
	{
		printf("input error: args 4 is too long!\n");
		exit(-1);
	}
	
	memset(message_s, 0, sizeof(message_s));
	memcpy(message_s, argv[3], strlen(argv[3]));
	printf("qyc, message will send == %s, len = %u\n", message_s, strlen(message_s));
	
 
	fd = init_serial_device(serial_node_name, baud);
	if (fd == -1) return -1;
	
	send_serial_data(fd, message_s);
	
	printf("qyc, end\n");
	return 0;
}
```

# java测试程序

* [AndroidSerialPort](https://github.com/AIlll/AndroidSerialPort)