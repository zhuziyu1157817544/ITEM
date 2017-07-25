/*************************************************************************
	> File Name: my_server.c
	> Author:yyc 
	> Mail:794990923@qq.com 
	> Created Time: 2016年08月09日 星期二 19时34分19秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "time.h"
#define SERV_PORT   8888    //服务器端口
#define QUEUE       100     //队列长度

#define  ENROOL       0         //注册
#define  LOGIN        1         //登录
#define  WHISPER      2         //私聊
#define  GROUPCHAT    3         //群聊
#define  CHATRECORD   4         //聊天记录
#define  SENDFILE     5         //发送文件
#define  OFFLINE      6         //离线消息
#define  SYSTEMMSG    9         //系统通知
#define  END          10        //退出

//数据传输结构体 
struct usersend 
{
    int  flag;                //标志位
    // char name[32];
    char accountnumber[32];   //帐号
    char sendobject[32];      //收件人帐号
    char data[1024];           //数据
    char time[40];            //时间
    struct usersend *next;
};
struct usersend *pheadoffline = NULL;

//存储帐号密码结构体
typedef struct userinfo 
{
    int  connector;          //套接字描述符
   // char name[32];
    char accountnumber[32];  //帐号
    char data[128];          //密码
    struct userinfo *next;
} userinfo;
struct userinfo *phead = NULL;

//存储好友结构体
typedef struct friend
{
    char accountnumber[32];
    struct friend *next;
} friend;
//friend friendinfo;
//struct friend *head = NULL;

int                optval;
int                conn_fd;       //套接字描述符


//函数声明
void my_error(const char* err_string, int line); 
void join();
void thread(void *arg);
void Enrool(int fd, struct usersend *datagram);
void Login(int fd, struct usersend *datagram);
void Whisper(int fd, struct usersend *datagram);
void Groupchat(int fd, struct usersend *datagram);
void Addfriends();
void Sendfile(int fd, struct usersend *datagram);
void send_info(int fd, struct usersend datagram);
int recv_info(int fd, struct usersend *datagram);
userinfo *read_inf();
void save_inf();
int sendmsgtoall(char *msg, int fd); 
char *my_time();
void End(int fd, struct usersend *datagram);


//自定义错误处理函数
void my_error(const char* err_string, int line) 
{
    fprintf(stderr, "line:%d", line);
    perror(err_string);
    exit(1);
}

//发送数据至客户端
void send_info(int fd, struct usersend datagram)
{
    if (send(fd, &datagram,sizeof(struct usersend), 0) < 0) {
        my_error("send", __LINE__);
    }
}

//接收客户端信息
int recv_info(int fd, struct usersend *datagram)
{
    int ret;
    memset(datagram, 0, sizeof(struct usersend));
    if ((ret = recv(fd, datagram, sizeof(struct usersend), 0)) < 0) {
        my_error("recv", __LINE__);
    } else if (ret == 0) {
        return(-1);    //异常断开连接
    }
}

//向文件中保存用户信息
void save_inf()
{ 
    int     file_fd;                 //文件描述符
    userinfo *p;
    //打开文件
    if ((file_fd = (open("user", O_WRONLY || O_TRUNC))) < 0) {
        my_error("open", __LINE__);
	return;
    }
    
    for (p = phead -> next; p != NULL; p = p -> next) {
        if ((write(file_fd, p, sizeof(struct userinfo))) <= 0) {
            my_error("write", __LINE__);
        }
    }

    close(file_fd);
}

//从文件中提取数据保存到链表
userinfo *read_inf()
{
    userinfo userinfomation;
    userinfo *pend, *ptemp;
    pend = phead;

    int     file_fd;                 //文件描述符
    int     size;                    //read返回值
    //打开文件
    if ((file_fd = (open("user", O_RDONLY))) < 0) {
        my_error("open", __LINE__);
        return NULL;
    }
    //读取数据
    while(1) {
        ptemp = (userinfo *)malloc(sizeof(userinfo));
        if ((size = (read(file_fd, &userinfomation, sizeof(struct userinfo)))) <= 0) {
            free(ptemp);
            return phead;
        }
        ptemp -> connector = -1;
        //strcpy(ptemp -> name, userinfomation.name);
        strcpy(ptemp -> accountnumber, userinfomation.accountnumber);
        strcpy(ptemp -> data, userinfomation.data);

        ptemp ->next = NULL;
        pend -> next = ptemp;
        pend = ptemp;
    }
    close(file_fd);
    return phead;
}


//选项功能处理
void thread(void *arg)
{
    int  fd;
    fd = conn_fd;
    
    //设置套接字属性，判断异常退出
    optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval, sizeof(int)) < 0) {
        my_error("setsockopt", __LINE__);
    }
    while(1) {
        struct usersend datagram;       //接收客户端信息
        if ((recv_info(fd, &datagram)) < 0) {
            End(fd, &datagram);  
            break;      //异常断开连接
        }
        switch(datagram.flag)
        {
            case ENROOL:     Enrool(fd ,&datagram);    break;
            case LOGIN:      Login(fd, &datagram);     break;
            case WHISPER:    Whisper(fd, &datagram);   break;
            case GROUPCHAT:  Groupchat(fd, &datagram); break;
            case SENDFILE:   Sendfile(fd, &datagram);  break;
            case END:        End(fd, &datagram);       break;
        }
    }

}

//注册函数
void Enrool(int fd,struct usersend *datagram)
{
    userinfo *p,*p1;

    for(p1 = phead -> next; p1 != NULL; p1 = p1 -> next) {
        if ((strcmp(p1 -> accountnumber, datagram -> accountnumber)) == 0) {
            printf("用户%s注册失败,该帐号已存在\n", datagram -> accountnumber);
            strcpy(datagram->data, "注册失败，帐号已存在");
            send_info(fd, *datagram);
            return ;
        }
    }
    
    p = (userinfo *)malloc(sizeof(userinfo));
    p -> connector = fd; 
    //strcpy(p -> name, datagram -> name);
    strcpy(p -> accountnumber, datagram -> accountnumber);
    strcpy(p -> data, datagram -> data);
    p->next = phead->next;
    phead->next = p;
    save_inf();                                   //将用户信息保存到文件
    
    printf("用户%s注册成功\n", datagram -> accountnumber);
   
    FILE *fpp;                                               //系统日志
    fpp = fopen("journal", "at+");
    if (fpp == NULL) {
        my_error("fopen", __LINE__);
    }
    fprintf(fpp, "%s\t用户%s注册成功\n",my_time(), datagram -> accountnumber);
    fclose(fpp);
    
    strcpy(datagram->data, "注册成功");
    send_info(fd, *datagram);
}

//登录函数
void Login(int fd,struct usersend *datagram) 
{
    userinfo *p;
    p = phead->next;
    char msg[] = "上线";
    while(p)
    {
        if ((strcmp(p -> accountnumber, datagram -> accountnumber)) == 0) {
            if (p -> connector > 0) {
                printf("该帐号已登录\n");
                strcpy(datagram -> data, "该帐号已登录，请检查输入帐号是否正确");
                send_info(fd, *datagram);
                break;
            }
            if ((strcmp(p -> data, datagram -> data)) == 0) {
                printf("帐号%s登录成功\n", datagram -> accountnumber);

                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t帐号%s登录成功\n",my_time(), datagram -> accountnumber);
                fclose(fpp);
                    
                strcpy(datagram->data,"登录成功");
                send_info(fd, *datagram);
                p->connector = fd;                                       //上线状态

                sendmsgtoall(msg, fd);                                   //向其他人提示上线
                

                struct usersend *ptemp = pheadoffline -> next, *qtemp = pheadoffline;                                  //查找是否存在离线消息
                struct usersend offline;
                if (pheadoffline -> next != NULL) {
                    while(ptemp) {
                        if ((strcmp(ptemp -> sendobject, datagram -> accountnumber)) == 0) {
                            strcpy(offline.accountnumber, ptemp -> accountnumber);
                            strcpy(offline.sendobject, ptemp -> sendobject);
                            offline.flag=ptemp->flag;                    //设置离线标志位
                            strcat(offline.data, ptemp -> data);
                            strcpy(offline.time, ptemp -> time);
                            send_info(fd, offline);
                            break;
                        }
                        qtemp = ptemp;
                        ptemp = ptemp->next;
                    }
                    if (ptemp -> next == NULL) {
                        qtemp -> next = NULL;
                        free(ptemp);
                    } else {
                        qtemp -> next = ptemp -> next;
                        free(ptemp);
                    }
                }
                return ;
            }
            else
            {
                printf("帐号%s登录失败， 密码错误\n", datagram -> accountnumber);
                
                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t帐号%s登录失败，密码错误\n",my_time(), datagram -> accountnumber);
                fclose(fpp);
                
                send_info(fd,*datagram);
                strcpy(datagram -> data, "登录失败，密码错误");
                return ;
            }
        }
        else
            p = p->next;
    }
    if(p == NULL)
    {
        strcpy(datagram -> data, "登录失败，帐号不存在");
        printf("帐号%s登录失败,该帐号不存在\n", datagram->accountnumber);
        send_info(fd, *datagram);
    }
}

//批量向所有用户发送信息
int sendmsgtoall(char *msg, int fd) 
{
    struct usersend datagram;
    userinfo *p;
    datagram.flag = SYSTEMMSG;
    strcpy(datagram.accountnumber,"系统");

    if (fd > 0){
    for(p = phead -> next; p != NULL; p = p -> next) {
        if (p -> connector == fd) {
            strcpy(datagram.data,p -> accountnumber);
	        strcat(datagram.data,msg);
	    break;
        }
    }
    }else
      strcpy(datagram.data,msg);

    for(p = phead -> next; p != NULL; p = p -> next) {
        if ((p -> connector != -1 )&&(p -> connector != fd)) {
            strcpy(datagram.sendobject, p -> accountnumber);
            send_info(p -> connector, datagram);
        }
    }
}
//私聊函数
void Whisper(int fd, struct usersend *datagram)
{
    userinfo *p = phead -> next;
    while(p)
    {
	    if ((strcmp(datagram -> sendobject, p -> accountnumber) == 0 )) {
            if (p -> connector > 0) {
	            printf("收到%s向 %s 发送的私聊消息\n",datagram->accountnumber,datagram->sendobject);

                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t收到%s向%s发送的私聊消息\n",my_time(), datagram-> accountnumber,datagram->sendobject);
                fclose(fpp);
                
                send_info(p -> connector, *datagram);
                return;
            } else {
                printf("用户%s不在线,用户%s发送的信息已转存为离线消息\n", p -> accountnumber, datagram -> accountnumber);
                
                struct usersend *ptemp = pheadoffline, *qtemp;                            //保存离线消息
                while(ptemp->next != NULL) {
                    ptemp = ptemp -> next;
                }
                qtemp = (struct usersend *)malloc(sizeof(struct usersend));   
                qtemp -> flag = datagram -> flag;
                strcpy(qtemp -> accountnumber, datagram -> accountnumber);
                strcpy(qtemp -> sendobject, datagram -> sendobject);
                strcpy(qtemp -> data, datagram -> data);
                strcpy(qtemp -> time, datagram -> time);
                qtemp->flag=OFFLINE;                                     //设置离线标志位
                ptemp->next=qtemp;
                qtemp -> next = NULL;

                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t用户%s不在线，用户%s发送离线信息\n",my_time(), p -> accountnumber, datagram->accountnumber);
                fclose(fpp);
                
                strcpy(datagram -> data, p -> accountnumber);
                strcat(datagram -> data, "用户不在线，发送的信息已转存为离线消息");
                send_info(fd, *datagram);
                return;
            }
        }
        else
            p = p->next;
    }
    if(p == NULL) {
        printf("用户%s不存在!  用户%s发送信息失败\n", datagram -> sendobject, datagram -> accountnumber);

        FILE *fpp;                                               //系统日志
        fpp = fopen("journal", "at+");
        if (fpp == NULL) {
            my_error("fopen", __LINE__);
        }
        fprintf(fpp, "%s\t用户%s不存在，用户%s发送信息失败\n",my_time(), datagram -> sendobject,datagram -> accountnumber);
        fclose(fpp);
 
        strcpy(datagram -> data, datagram -> sendobject);
        strcat(datagram -> data, "用户不存在，发送信息失败");
        send_info(fd, *datagram);
        return;   
    }

}

//群聊函数
void Groupchat(int fd, struct usersend *datagram)
{
    userinfo *p = phead -> next;
    while(p)
    {
	    if ((strcmp(datagram -> sendobject, p -> accountnumber) == 0 )) {
            if (p -> connector > 0) {

	            printf("收到%s向%s 发送的群聊消息\n",datagram->accountnumber,datagram->sendobject);
              
                FILE *fpp;                                       //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t收到%s向%s发送的群聊消息\n",my_time(), datagram -> accountnumber, datagram -> sendobject);
                fclose(fpp);
               
                send_info(p -> connector, *datagram);            //发送群聊消息   
                return;
            } else {
                printf("用户%s不在线,用户%s发送的信息已转存为离线消息\n", p -> accountnumber, datagram -> accountnumber);
                
                struct usersend *p = pheadoffline -> next;                            //保存离线消息
                while(p != NULL) {
                    p = p -> next;
                }
                p = (struct usersend *)malloc(sizeof(struct usersend));   
                p -> flag = datagram -> flag;
                strcpy(p -> accountnumber, datagram -> accountnumber);
                strcpy(p -> sendobject, datagram -> sendobject);
                strcpy(p -> data, datagram -> data);
                strcpy(p -> time, datagram -> time);
                p -> next = NULL;

                FILE *fpp;                                       //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t用户%s不在线，用户%s发送的信息已转存为离线消息\n",my_time(), p -> accountnumber, datagram -> accountnumber);
                fclose(fpp);
                
                strcpy(datagram -> data, p -> accountnumber);
                strcat(datagram -> data, "用户不在线，发送的信息已转存为离线消息");
                send_info(fd, *datagram);                        //向发送信息用户反馈发送离线消息信息
                return;
            }
        }
        else
            p = p->next;
    }
    if(p == NULL) {
        printf("用户%s不存在! 用户%s发送群聊信息失败\n", datagram -> sendobject, datagram -> accountnumber);

        FILE *fpp;                                               //系统日志
        fpp = fopen("journal", "at+");
        if (fpp == NULL) {
            my_error("fopen", __LINE__);
        }
        fprintf(fpp, "%s\t用户%s不存在! 用户%s发送群聊信息失败\n",my_time(), datagram -> sendobject, datagram -> accountnumber);
        fclose(fpp);
        
        strcpy(datagram -> data, datagram -> sendobject);
        strcat(datagram -> data, "用户不存在，发送群聊信息失败");    //向发送信息用户反馈发送失败信息
        send_info(fd, *datagram);
        return;
    }
}
//发送文件
void Sendfile(int fd,struct usersend *datagram)
{
    userinfo *p = phead -> next;
    while(p)
    {
	    if ((strcmp(datagram -> sendobject, p -> accountnumber) == 0 )) {
            if (p -> connector > 0) {
	            printf("收到%s向 %s 发送的文件\n",datagram->accountnumber,datagram->sendobject);

                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t收到%s向%s发送的文件\n",my_time(), datagram-> accountnumber,datagram->sendobject);
                fclose(fpp);
                send_info(p -> connector, *datagram);

                while(1) {
                    struct usersend filedatagram;       
                    recv_info(fd, &filedatagram);
                    send_info(p -> connector, filedatagram);
                    if((sizeof(filedatagram.data)) < 1024) {
                        break;
                    }
                }
                return;
            } else {
                printf("用户%s不在线,用户%s发送文件失败\n", p -> accountnumber, datagram -> accountnumber);
                
                FILE *fpp;                                               //系统日志
                fpp = fopen("journal", "at+");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fprintf(fpp, "%s\t用户%s不在线，用户%s发送文件失败\n",my_time(), p -> accountnumber, datagram->accountnumber);
                fclose(fpp);
                
                strcpy(datagram -> data, p -> accountnumber);
                strcat(datagram -> data, "用户不在线，发送文件失败");
                send_info(fd, *datagram);
                return;
            }
        }
        else
            p = p->next;
    }
    if(p == NULL) {
        printf("用户%s不存在!  用户%s发送文件失败\n", datagram -> sendobject, datagram -> accountnumber);

        FILE *fpp;                                               //系统日志
        fpp = fopen("journal", "at+");
        if (fpp == NULL) {
            my_error("fopen", __LINE__);
        }
        fprintf(fpp, "%s\t用户%s不存在，用户%s发送文件\n",my_time(), datagram -> sendobject,datagram -> accountnumber);
        fclose(fpp);
 
        strcpy(datagram -> data, datagram -> sendobject);
        strcat(datagram -> data, "用户不存在，发送文件失败");
        send_info(fd, *datagram);
        return;   
    }

}

//获取时间
char *my_time()
{
    time_t t;
    time(&t);
    return ctime(&t);
}

//退出
void End(int fd, struct usersend *datagram)
{
    userinfo *p;

    //datagram -> flag = END;
    //send_info(fd, *datagram);

    sendmsgtoall("下线", fd);
    //将该用户置为下线状态
    for(p = phead -> next; p!= NULL; p = p -> next) {
        if (fd == p -> connector) {
            p -> connector = -1;
            break;
        }
    }
    if(p == NULL)
    {
        free(p);
        close(fd);
        pthread_exit(0);
    }
    FILE *fpp;                                               //系统日志
    fpp = fopen("journal", "at+");
    if (fpp == NULL) {
        my_error("fopen", __LINE__);
    }
    fprintf(fpp, "%s\t%s用户下线\n",my_time(), p -> accountnumber);
    fclose(fpp);

    printf("%s\t%s用户下线\n", my_time(), p -> accountnumber);

    close(fd);
    pthread_exit(0);
}

void join()
{
    int                status, sock_fd;
    struct sockaddr_in serv_addr;     //服务器端套接字地址结构体
    pthread_t          thid;
    socklen_t          cli_len;       //客户端套接字地址结构体长度
    struct sockaddr_in cli_addr;      //客户端套接字地址结构体

    //初始化服务器端地址结构
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

    phead = read_inf();               //读取用户信息

    //创建一个套接字
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        my_error("socket", __LINE__);
    }

    //设置套接字使之可以重新绑定接口
    optval = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(int)) < 0) {
        my_error("setsockopt", __LINE__);
    }
    
    //绑定套接字到本地端口
    if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
        my_error("bind", __LINE__);
    }
    
    //将套接字化为监听套接字
    if (listen(sock_fd, QUEUE) < 0) {
        my_error("listen", __LINE__);
    }


    cli_len = sizeof(struct sockaddr_in);
    while(1) {
        //接受客户端的请求
        conn_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (conn_fd < 0) {
            my_error("accept", __LINE__);
        }
        printf("accept a new client, IP: %s\n", inet_ntoa(cli_addr.sin_addr));
    
        FILE *fpp;
        fpp = fopen("journal", "at+");
        if (fpp == NULL) {
            my_error("fopen", __LINE__);
        }
        fprintf(fpp, "%s\taccept a new client, IP:%s\n",my_time(), inet_ntoa(cli_addr.sin_addr));
        fclose(fpp);

        //创建线程处理客户端请求
        if (pthread_create(&thid, NULL, (void *)thread, NULL) != 0) {
            printf("pthread error!\n");
            close(conn_fd);
        } 
    }
}


int main()
{
    phead = (struct userinfo *)malloc(sizeof( struct userinfo ));                   //读取用户信息链表
    phead -> next = NULL;
    
    pheadoffline = (struct usersend *)malloc(sizeof(struct usersend));              //读取离线消息链表
    pheadoffline -> next = NULL;
    
    join();
    return 0;
}
