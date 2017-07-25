/******************************************************************
	> Author:yyc 
	> Mail:794990923@qq.com 
	> Created Time: 2016年08月10日 星期三 14时02分45秒
 ************************************************************************/

#include <stdio.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define SERV_PORT    8888              //服务器端口
#define SERV_IP      "127.0.0.1"       //服务器IP地址

#define  ENROOL       0          //注册
#define  LOGIN        1          //登录
#define  WHISPER      2          //私聊
#define  GROUPCHAT    3          //群聊
#define  CHATRECORD   4          //聊天记录
#define  SENDFILE     5          //发送文件
#define  OFFLINE      6          //离线消息
#define  SYSTEMMSG    9          //系统通知
#define  END          10         //退出 
//数据传输结构体
struct usersend {

    int  flag;
    char accountnumber[32];     //帐号
    char sendobject[32];        //收件人帐号
    char data[1024];
    char time[40];          //保存时间
    struct usersend *next;  
};

struct usersend datagram;

int                conn_fd;      //套接字描述符

//函数声明
void my_error(const char *err_string, int line);
void join();
void menu();
void get_info();
void optioninterface();
void Enrool();
void Login();
void Whisper();
void Groupchat();
void Chatrecord();
void Sendfile();
void send_info();
void recv_info();
void thread(void *arg);
char *my_time();
void End();

//自定义错误处理函数
void my_error(const char *err_string, int line)
{
    fprintf(stderr, "line: %d", line);
    perror(err_string);
    exit(1);
}

//获取用户登录注册信息并存入结构体
void get_info()
{
    int i;
    char c;

/*    //获取用户昵称
    if (datagram  -> flag == 0) {
        i = 0;
        printf("请输入用户昵称:\n");
        while(((c = getchar()) != '\n') && (c != EOF) && (i < 30)) {
            datagram -> name[i++] = c;
        }
        datagram -> name[i++] = '\n';
        datagram -> name[i++] = '\0';
    }
*/
    //获取用户帐号
    i = 0;
    printf("请输入用户帐号:\n");
    while(((c = getchar()) != '\n') && (c != EOF) && (i < 30)) {
        datagram.accountnumber[i++] = c;
    }
    datagram.accountnumber[i++] = '\0';

    //获取用户密码
    char buf[32];
    char *password1;
    char *password2;
    password1 = getpass("请输入密码:");
    strcpy(buf, password1);
    //判断是否为注册，二次输入密码
    if (datagram.flag == 0) {   
        password2 = getpass("请再次输入密码:");
        if (strcmp(buf,password2) != 0) {
            printf("两次密码输入不一致！\n");
            printf("注册失败！\n");
            exit(-1);
        }
    }

    strcpy(datagram.data, buf);
    /*
    if (userinfo.flag == 0) {
        i = 0;
        printf("请再次输入用户密码:\n");
        while(((c = getchar()) != '\n') && (c != EOF) && (i < 30)) {
            datagram -> password[i++] = c;
        }
        datagram -> password[i++] = '\0';
    }
    */
}


//发送信息至服务器端
void send_info()
{   
    if (send(conn_fd, &datagram, sizeof(struct usersend), 0) < 0) {
        my_error("send", __LINE__);
    }
}


//接收服务器发送的信息
void recv_info()
{
    int i, ret;
    
   if ((ret = recv(conn_fd, &datagram, sizeof(struct usersend), 0)) < 0) {
        printf("error receive\n");
        exit(1);
    } else if (ret == 0) {
        printf("服务器断开连接!\n");      //异常断开连接
    }
}

//建立与服务器的连接
void join()
{
    struct sockaddr_in serv_addr;

    //初始化服务器端地址结构
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));
  
    //创建一个套接字
    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd < 0) {
        my_error("socket", __LINE__);
    }  

    //设置套接字属性，判断异常退出
    int optval = 1;
    if (setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval, sizeof(int)) < 0) {
        my_error("setsockopt", __LINE__);
    }

    //建立连接
    if (connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
        my_error("connect", __LINE__);
    } else {
        printf("connection successful!  >_<\n");
    }
    menu();
}

//开始界面 
void menu()
{
    int choose;
    char opinion[5];
    char c;
    printf("                           聊天室                          \n");
    printf("===========================================================\n");
    printf("|---------------------------------------------------------|\n");
    printf("|            0.注册          |         1.登录             |\n");
    printf("|---------------------------------------------------------|\n");
    
    do {
        printf("请选择所需服务:\n");
        scanf("%d", &choose);
        if (choose < 0 || choose > 1) {
            printf("输入错误!\n");
            printf("是否重新输入?  y/n : ");
            scanf("%s",opinion);
        }
        else
            strcpy(opinion,"n");
    } while ((strcmp(opinion,"y")) == 0);

    while((c = getchar()) != '\n')
      continue;
    
    switch (choose) 
    {
        case ENROOL: Enrool();     break;
        case LOGIN:  Login();      break;
    }
}

//功能选择界面
void optioninterface()
{
    int        i, choose;
    char       opinion[5];
    
    pthread_t  thid;
    if ((pthread_create(&thid, NULL, (void *)thread, NULL)) != 0) {
        printf("pthread error!\n");
    }

    while(1)
    {
        printf("                       欢迎进入聊天室                      \n");
        printf("===========================================================\n");
        printf("|---------------------------------------------------------|\n");
        printf("|            2.私聊          |         3.群聊             |\n");
        printf("|---------------------------------------------------------|\n");
        printf("|            4.聊天记录      |         5.发送文件         |\n");
        printf("|---------------------------------------------------------|\n");
        printf("|                         10.退出                         |\n");
        printf("|---------------------------------------------------------|\n");

    
    
        do {
            printf("请选择所需服务:\n");
            scanf("%d", &choose);
            if (choose < 2 || choose > 10) {
                printf("输入错误!\n");
                printf("是否重新输入?  y/n : ");
                scanf("%s",opinion);
            }
            else
                strcpy(opinion,"n");
        } while ((strcmp(opinion,"y")) == 0);
    
    
        switch (choose) 
        {
            case WHISPER:    Whisper();    break;
            case GROUPCHAT:  Groupchat();  break;
            case CHATRECORD: Chatrecord(); break;
            case SENDFILE:   Sendfile();   break;
            case END:        End();        break;
        }
    }
}

//创建线程，等待接受服务器所发送的信息
void thread(void *arg)
{
    int i, ret;
    struct usersend recvinfomation;
    //设置套接字属性，判断异常退出
    int optval = 1;
    if (setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval, sizeof(int)) < 0) {
        my_error("setsockopt", __LINE__);
    }
   while(1)
    {
        memset(&recvinfomation,0,sizeof(struct usersend));
        if ((ret = recv(conn_fd, &recvinfomation, sizeof(struct usersend), 0)) < 0) {
            printf("error receive\n");
            exit(1);
        }
        if (ret == 0) {
            printf("服务器断开连接!\n");   //异常断开连接
            exit(-1);
        }
        if ((strcmp(recvinfomation.accountnumber, datagram.accountnumber)) == 0) {
            printf("收到来自服务器的提示信息:%s\n", recvinfomation.data);
        } else {
            if(recvinfomation.flag == 2) {
                printf("收到来自%s的私聊消息:%s\n", recvinfomation.accountnumber, recvinfomation.data);

                FILE *fpp;                 //聊天记录
                char buf[32];
                strcpy(buf, recvinfomation.sendobject);
                strcat(buf, recvinfomation.accountnumber);
                fpp = fopen(buf, "at");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fwrite(&recvinfomation,sizeof(struct usersend),1,fpp);
                fclose(fpp);
            }
            if(recvinfomation.flag == 3) {
                printf("收到来自%s的群聊消息:%s\n", recvinfomation.accountnumber, recvinfomation.data);
                
                FILE *fpp;                 //聊天记录
                char buf[32];
                strcpy(buf, recvinfomation.sendobject);
                strcat(buf, recvinfomation.accountnumber);
                fpp = fopen(buf, "at");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fwrite(&recvinfomation,sizeof(struct usersend),1,fpp);
                fclose(fpp);
            }
            if(recvinfomation.flag == 9) {
                usleep(10);
                printf("收到来自%s的消息：%s\n", recvinfomation.accountnumber, recvinfomation.data);
            }
            if(recvinfomation.flag == 6){
                printf("收到来自%s的离线消息:%s\n",recvinfomation.accountnumber,recvinfomation.data);

                FILE *fpp;                 //聊天记录
                char buf[32];
                strcpy(buf, recvinfomation.sendobject);
                strcat(buf, recvinfomation.accountnumber);
                fpp = fopen(buf, "at");
                if (fpp == NULL) {
                    my_error("fopen", __LINE__);
                }
                fwrite(&recvinfomation,sizeof(struct usersend),1,fpp);
                fclose(fpp);
            }
            if(recvinfomation.flag == 5){
                strcat(recvinfomation.data,"JS");
                printf("收到来自%s发送的文件%s\n", recvinfomation.accountnumber,recvinfomation.data);
                FILE *fpp;
                fpp = fopen(recvinfomation.data, "at+");

                while(1) {
               //     struct usersend filedatagram;
                    recv_info();
                    printf("recvinfomation.data: %s\n", datagram.data);

                    fwrite(datagram.data, 1024, 1, fpp);
                    if ((strlen(datagram.data)) < 1024) {
                        break;
                    }
                }
                
                fclose(fpp);
            }
        }
    }
}
//注册函数
void Enrool()
{
    datagram.flag = 0;     //设置标志位

    get_info(&datagram);
    send_info();
    recv_info(&datagram);
    printf("%s\n", datagram.data);
    if ((strcmp(datagram.data, "注册成功")) == 0) {
        optioninterface();
    } else {
        menu();
    }
}
//登录函数
void Login()
{
    datagram.flag = 1;     //设置标志位

    get_info(&datagram);
    send_info();
    recv_info(&datagram);
    printf("%s\n", datagram.data);
    if ((strcmp(datagram.data, "登录成功")) == 0) {
        optioninterface();
    } 
    if ((strcmp(datagram.data, "该帐号已登录，请检查输入帐号是否正确")) == 0) {
        menu();
    }
    else {
        menu();
    }
}
//私聊函数
void Whisper()
{
    datagram.flag = 2;     //设置标志位
    printf("请输入好友帐号:\n");
    scanf("%s", datagram.sendobject);
    printf("请输入要发送的信息\n");
    getchar();
    gets(datagram.data);
    strcpy(datagram.time,my_time());
    send_info();
    FILE *fpp;                 //聊天记录
    char buf[32];
    strcpy(buf, datagram.accountnumber);
    strcat(buf, datagram.sendobject);
    fpp = fopen(buf, "at");
    if (fpp == NULL) {
        my_error("fopen", __LINE__);
    }
    fwrite(&datagram,sizeof(struct usersend),1,fpp);
    fclose(fpp);

}
//群聊函数
void Groupchat()
{
    int  i, max;
    char groupchat[10][32];
    datagram.flag = 3;     //设置标志位
 
    printf("请输入群聊人数:\n");
    scanf("%d", &max);
    printf("请输入群聊好友帐号:\n");
    for (i = 0; i < max; i++) {
        scanf("%s", groupchat[i]);
    }
    printf("请输入群聊消息:\n");
    getchar();
    gets(datagram.data);
    for (i = 0; i < max; i++) {
        strcpy(datagram.sendobject, groupchat[i]);
        strcpy(datagram.time,my_time());
        send_info();
        
        FILE *fpp;                 //聊天记录
        char buf[32];
        strcpy(buf, datagram.accountnumber);
        strcat(buf, datagram.sendobject);
        fpp = fopen(buf, "at");
        if (fpp == NULL) {
            my_error("fopen", __LINE__);
        }
        fwrite(&datagram,sizeof(struct usersend),1,fpp);
        fclose(fpp);
    }
}
//聊天记录
void Chatrecord()
{
    datagram.flag = 4;     //设置标志位
    struct usersend aaa;
    char time[50];
    char name_from[20];
    //char friendname[32];

    printf("请输入要查看的好友帐号:\n");
    scanf("%s",datagram.sendobject);
    char filename[32];
    strcpy(filename,datagram.accountnumber);
    strcat(filename,datagram.sendobject);
    FILE *fpp;
    fpp = fopen(filename,"rt");
    while(fread(&aaa,sizeof(struct usersend),1,fpp))
    {
        printf("%s",aaa.time);
        printf("用户%s:%s\n",aaa.accountnumber,aaa.data);
        memset(&aaa,0,sizeof(struct usersend));
    }
    fclose(fpp);
}
//获取时间
char *my_time()
{
    time_t t;
    time(&t);
    return ctime(&t);
}

//发送文件
void Sendfile()
{
    datagram.flag = 5;     //设置标志位
    FILE *fpp;
    char filename[32];

    fflush(stdin);
    
    printf("请输入用户帐号:");
    scanf("%s", datagram.sendobject);

    printf("请输入要发送的文件名:");
    scanf("%s", filename);

    strcpy(datagram.data, filename);
    send_info();

    memset(&datagram.data, 0, sizeof(struct usersend));

    fpp = fopen(filename, "r");
    while(!feof(fpp)){

        fread(datagram.data, 1024, 1, fpp);

        send_info();
    }
    fclose(fpp);
    
}

//退出
void End()
{  
    datagram.flag = 10;      //设置标志位
    send_info();    
    close(conn_fd);
    exit(0);
}

int main()
{
    join();
    return 0;
}
