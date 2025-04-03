#include<stdio.h>
#include<cstring>
#include<cstdlib>
#include<iostream>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
using namespace std;
int main (int argc,char *argv[])
{
    if(argc!=3)
    {
        cout<<"Using:  ./client ip 5005\n";
        return 0;
    }

    //第一步，创建客户端的socket
    int sockfd =socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sockfd==-1)
    {
        perror("socket");return -1;
    }

    //第二步，向服务器发起连接请求
    // struct sockaddr_in {
    //     short int sin_family;      // 协议族，通常为 AF_INET
    //     unsigned short int sin_port; // 端口号（网络字节顺序）
    //     struct in_addr sin_addr;   // IP地址
    //     unsigned char sin_zero[8]; // 填充字段，使结构体大小与 sockaddr 一致
    // };
    struct sockaddr_in cliaddr;               //存放协议、端口和IP地址的结构体
    memset(&cliaddr,0,sizeof(cliaddr));
    cliaddr.sin_family =AF_INET;
    cliaddr.sin_port = htons(atoi(argv[2]));         //16位主机字节顺序（Host Byte Order）的整数转换为网络字节顺序
    // struct hostent {
    //     char  *h_name;        // 官方主机名
    //     char **h_aliases;     // 主机的别名列表
    //     int    h_addrtype;    // 地址类型（例如，AF_INET）
    //     int    h_length;      // 地址长度（例如，IPv4的长度为4）
    //     char **h_addr_list;   // 网络地址列表（每个地址的长度由h_length确定）
    // };
    //其中有宏定义为 #define h_addr h_addr_list[0]
    struct hostent* h;                         //存放服务器端IP的结构体
    if((h=gethostbyname(argv[1]))==nullptr)    //将域名解析为IP地址
    {
        cout<<"gethostbyname failed()\n";
        close(sockfd);
        return -1;
    }
    memcpy(&cliaddr.sin_addr,h->h_addr,h->h_length);
    if(connect(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr))==-1)      //发起连接请求
    {
        perror("connect");close(sockfd);return -1;
    }

    char buffer[1024];
    int tmp = 1;
    int iret;
    while(true)
    {
        memset(buffer,0,sizeof(buffer));
        snprintf(buffer,1024,"这是第%d个消息.\n",tmp++);
        if(send(sockfd,buffer,sizeof(buffer),0)<=0)
        {
            perror("send");close(sockfd);return -1;
        }
        memset(buffer,0,sizeof(buffer));
        if((iret=recv(sockfd,buffer,sizeof(buffer),0))<=0)
        {
            cout<<"iret="<<iret<<endl;
        }
        cout<<"接收"<<buffer<<endl;
        sleep(2);
    }
    close(sockfd);
    return 0;
}