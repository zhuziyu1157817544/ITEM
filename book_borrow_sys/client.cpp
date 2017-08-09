/*************************************************************************
	> File Name: client.cpp
	> Author: yanyuchen
	> Mail: 794990923@qq.com
	> Created Time: 2017年08月01日 星期二 08时21分29秒
 ************************************************************************/
#include<iostream>
#include<string.h>
#include<math.h>
#include<sys/signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include <termios.h>
#include<assert.h>

#include <json/json.h>
#include"protocol.h"

#define PORT 6666   //服务器端口


//网络操作码flags:
    //定义在protocol头文件中

using namespace std;


class TCPClient
{
    public:

    TCPClient(int argc ,char** argv);
    ~TCPClient();

    char data_buffer[10000];    //存放发送和接收数据的buffer


    //向服务器发送数据
    bool send_to_serv(int datasize,uint16_t wOpcode);
    //从服务器接收数据
    bool recv_from_serv();
    void run(TCPClient & client); //主运行函数


    private:

    int conn_fd; //创建连接套接字
    struct sockaddr_in serv_addr; //储存服务器地址

};




TCPClient::TCPClient(int argc,char **argv)  //构造函数
{

    if(argc!=3)    //检测输入参数个数是否正确
    {
        cout<<"Usage: [-a] [serv_address]"<<endl;
        exit(1);
    }

    memset(data_buffer,0,sizeof(data_buffer));  //初始化buffer

    //初始化服务器地址结构
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);
    
    //从命令行服务器地址
    for(int i=0;i<argc;i++)
    {
        if(strcmp("-a",argv[i])==0)
        {

            if(inet_aton(argv[i+1],&serv_addr.sin_addr)==0)
            {
                cout<<"invaild server ip address"<<endl;
                exit(1);
            }
            break;
        }
    }

    //检查是否少输入了某项参数
    if(serv_addr.sin_addr.s_addr==0)
    {
        cout<<"Usage: [-a] [serv_address]"<<endl;
        exit(1);
    }

    //创建一个TCP套接字
    conn_fd=socket(AF_INET,SOCK_STREAM,0);


    if(conn_fd<0)
    {
        my_err("connect",__LINE__);
    }
    

    //向服务器发送连接请求
    if(connect(conn_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0)
    {
        my_err("connect",__LINE__);
    }

}

TCPClient::~TCPClient()  //析构函数
{
    close(conn_fd);

}

bool TCPClient::send_to_serv(int datasize,uint16_t wOpcode) //向服务器发送数据
{
    NetPacket send_packet;   //数据包
    send_packet.Header.wDataSize=datasize+sizeof(NetPacketHeader);  //数据包大小
    send_packet.Header.wOpcode=wOpcode;


    memcpy(send_packet.Data,data_buffer,datasize);  //数据拷贝


    if(send(conn_fd,&send_packet,send_packet.Header.wDataSize,0))
    return true;
    else
    return false;

}

bool TCPClient::recv_from_serv()   //从服务器接收数据
{
    int nrecvsize=0; //一次接收到的数据大小
    int sum_recvsize=0; //总共收到的数据大小
    int packersize;   //数据包总大小
    int datasize;     //数据总大小


    memset(data_buffer,0,sizeof(data_buffer));   ///初始化buffer

      while(sum_recvsize!=sizeof(NetPacketHeader))
    {
        nrecvsize=recv(conn_fd,data_buffer+sum_recvsize,sizeof(NetPacketHeader)-sum_recvsize,0);
        if(nrecvsize==0)
        {
            //服务器退出;
            return false;
        }
        if(nrecvsize<0)
        {
            cout<<"从客户端接收数据失败"<<endl;
            return false;
        }
        sum_recvsize+=nrecvsize;

    }



    NetPacketHeader *phead=(NetPacketHeader*)data_buffer;
    packersize=phead->wDataSize;  //数据包大小
    datasize=packersize-sizeof(NetPacketHeader);     //数据总大小




    while(sum_recvsize!=packersize)
    {
        nrecvsize=recv(conn_fd,data_buffer+sum_recvsize,packersize-sum_recvsize,0);
        if(nrecvsize==0)
        {
            cout<<"从客户端接收数据失败"<<endl;
            return false;
        }
        sum_recvsize+=nrecvsize;
    }

    return true;

}

//函数声明:
bool inputpasswd(string &passwd);   //无回显输入密码
void Login_Register(TCPClient client);    //登录注册函数
void menu(TCPClient client);  //用户菜单函数
void Admin_menu(TCPClient &client); //管理员菜单函数


bool inputpasswd(string &passwd)   //无回显输入密码
{
    struct termios tm,tm_old;
    int fd = STDIN_FILENO, c,d;
    if(tcgetattr(fd, &tm) < 0)
    {
        return false;
    }
    tm_old = tm;
    cfmakeraw(&tm);
    if(tcsetattr(fd, TCSANOW, &tm) < 0)
    {
        return false;
    }
    cin>>passwd;
    if(tcsetattr(fd, TCSANOW, &tm_old) < 0)
    {
        return false;
    }
    return true;
}

void Login_Register(TCPClient client)   //登录注册函数
{

    string choice;           //记录选择
    int choice_right=1;     //判断输入选项是否正确
    string number,nickname,sex,address,birthdate, phone;               
    string passwd,passwd1;
    Json::Value accounts;



    cout<<"\t\t欢迎使用图书借阅系统:"<<endl;
    cout<<"\t\t\t1.登录"<<endl;
    cout<<"\t\t\t2.注册"<<endl;
    cout<<"\t\t\t3.退出"<<endl;
    cout<<"\n请输入选择序号:";

    while(choice_right)
    {
        cin>>choice;
        if(choice=="1")  //登录
        {
            choice_right=0;
            cout<<"请输入帐号:";
            cin>>number;
            cout<<"请输入密码:"; 
            inputpasswd(passwd);
            cout<<endl;
            accounts["number"]=number.c_str();           //加入json对象中
            accounts["passwd"]=passwd.c_str();
            string out=accounts.toStyledString();
            memcpy(client.data_buffer,out.c_str(),out.size());   //拷贝到数据buffer
            if(client.send_to_serv(out.size(),LOGIN)==false)  //向服务器发送数据
            {
                cout<<"向服务器发送数据失败"<<endl;
                return;
            }
            if(client.recv_from_serv()==false)   //从服务器接收数据
            {
                cout<<"从服务器接收数据失败"<<endl;
                return;
            }

            NetPacketHeader *phead=(NetPacketHeader*)client.data_buffer;
            if(phead->wOpcode==LOGIN_YES) //登录成功
            {
                cout<<"登录成功"<<endl;
                if (accounts["number"] == "admin") {          //判断是否为管理员
                    Admin_menu(client);
                }
                menu(client);
            }
            else
            {
                cout<<"登录失败，帐号或密码错误"<<endl;
            }


        }
        else if(choice=="2")  //注册
        {
            choice_right=0;
            cout<<"请输入要注册的帐号:";
            cin>>number;
            cout<<"请输入昵称:";
            cin>>nickname;
            cout<<"请输入性别:";
            cin>>sex;
            cout<<"请输入地址:";
            cin>>address;
            cout<<"请输入生日:";
            cin>>birthdate;
            cout<<"请输入手机号码:";
            cin>>phone;
            cout<<"请设置密码:";
            inputpasswd(passwd);
            cout<<endl;
            cout<<"请再次输入密码:";
            inputpasswd(passwd1);
            cout<<endl;
            while(passwd!=passwd1)
            {
                cout<<"两次输入的密码不同，请重新设置密码。"<<endl;
                cout<<"请设置密码:";
                inputpasswd(passwd);
                cout<<endl;
                cout<<"请再次输入密码:";
                inputpasswd(passwd1);
                cout<<endl;
            }
            accounts["number"]=number.c_str();
            accounts["nickname"]=nickname.c_str();
            accounts["sex"]=sex.c_str();
            accounts["address"]=address.c_str();
            accounts["birthdate"]=birthdate.c_str();
            accounts["phone"]=phone.c_str();
            accounts["passwd"]=passwd.c_str();
            string out=accounts.toStyledString();
            memcpy(client.data_buffer,out.c_str(),out.size());  //拷贝到数据buffer中


            if(client.send_to_serv(out.size(),REGISTER)==false)  //向服务器发送数据
            {
                cout<<"向服务器发送数据失败"<<endl;
                return;
            }
            if(client.recv_from_serv()==false)   //从服务器接收数据
            {
                cout<<"从服务器接收数据失败"<<endl;
                return;
            }

            NetPacketHeader *phead=(NetPacketHeader*)client.data_buffer;
            if(phead->wOpcode==REGISTER_YES)  //注册成功
            {
                cout<<"注册成功"<<endl;
                Login_Register(client);   
            }
            else
            {
                cout<<"注册失败,该帐号已被注册"<<endl;
            }
            
        }
            
        else if(choice=="3")
        {
            cout<<"Bye"<<endl;
            exit(0);
        }
        else
        {
            cout<<"输入格式错误，请重新输入"<<endl;
        }
    }
}

void Personal_data(TCPClient client)
{
    if(client.send_to_serv(0, PERSONAL_DATA)==false)  //向服务器发送数据失败
    {
        cout<<"向服务器发送数据失败"<<endl;
        return;
    }
    cout<<"recv_from_serv"<<endl;////////////

    if(client.recv_from_serv()==false)   //从服务器接收数据
    {
        cout<<"从服务器接收数据失败"<<endl;
        return;
    }
    
    cout<<"recv end"<<endl;/////////

    NetPacket *phead=(NetPacket*)client.data_buffer;
    
    Json::Value accounts;
    Json::Reader reader;
    string str(phead->Data);
     
    if (reader.parse(str,accounts) < 0)
    {
        cout << "json解析失败" << endl;
    }
    else {
        cout << "账号：" << accounts["number"].asString()<< endl;
        cout << "密码：" << accounts["passwd"].asString()<< endl;
        cout << "昵称：" << accounts["nickname"].asString()<< endl;
        cout << "性别：" << accounts["sex"].asString()<< endl;
        cout << "地址：" << accounts["address"].asString()<< endl;
        cout << "生日：" << accounts["birthdate"].asString()<< endl;
        cout << "电话：" << accounts["phone"].asString()<< endl;
    }
    cout << "按任意键返回菜单\n" << endl;
    getchar(); 
    getchar(); 
    menu(client);
}
bool add_books_info(TCPClient &client)
{
    bool non_stop = true;
    string yes_or_no;
    Json::Value book;
    string ISBN,book_name,publish_house,author,count,stat;

    while(non_stop){
        cout << endl;
        cout << endl;
        cout << "是否开始本次录入[yes/no]:";
        cin >> yes_or_no;
        if(yes_or_no != "yes"){
            cout << "您输入的选项不是yes,已经退出上线图书功能" << endl;
            non_stop = false;
            continue;
        }
        cout << "请输入图书ISBN:";
        cin >> ISBN;
        cout << "请输入图书名称:";
        cin >> book_name;
        cout << "请输入图书出版社:";
        cin >> publish_house;
        cout << "请输入图书作者:";
        cin >> author;
        cout << "请输入图书数量:";
        cin >> count;
        do{
            cout << "请输入图书是否可借阅[yes/no]:";
            cin >> stat;
        }while((stat != "yes")&& (stat != "no"));
        
        book["ISBN"] = ISBN.c_str();
        book["book_name"] = book_name.c_str();
        book["publish_house"] = publish_house.c_str();
        book["author"] = author.c_str();
        book["count"] = count.c_str();
        book["stat"] = stat.c_str();

        string out = book.toStyledString();
        memcpy(client.data_buffer,out.c_str(),out.size());
        if(client.send_to_serv(out.size(),ADD_BOOKS_INFO) == false){
            cout << "发送数据失败!" << endl;
            return false;
        }
        if(client.recv_from_serv() == false){
             cout << "接收数据失败"<<endl;
             return false;
        }else{
            //cout << "所接受的数据是:";
            //cout << client.data_buffer << endl;
            NetPacketHeader *phead = (NetPacketHeader*)client.data_buffer;
            if(phead -> wOpcode == ADD_BOOKS_INFO_YES){
                cout << "书籍添加成功" << endl;
            }else{
                cout << "书籍添加失败,原因可能是书籍在数据库中已存在" << endl;
                continue;
            }
        }
        cout << endl;
    }
    return true;
}

void del_isbn(TCPClient &client)
{
    
    bool non_stop = true;
    string temp_isbn1;
    string temp_isbn2;
    while(non_stop){
        cout << "请输入ISBN"<<endl;
        cin >> temp_isbn1;
        cout << "请再次输入ISBN确认"<<endl;
        cin >> temp_isbn2;
        if(temp_isbn1 == temp_isbn2){
            cout << "确认成功,正在删除..."<<endl;
            break;
        }
        else{
            cout << "两次输入不一致，请重新输入"<< endl;
            continue;
        }
    }
    Json:: Value book;
    string nu("");
    book["ISBN"] = temp_isbn1.c_str();
    book["book_name"] = nu.c_str();
    book["publish_house"] = nu.c_str();
    book["author"] = nu.c_str();
    book["count"] = nu.c_str();
    book["stat"] = nu.c_str();

    string out = book.toStyledString();
    memcpy(client.data_buffer,out.c_str(),out.size());
    if(client.send_to_serv(out.size(),DEL_BOOKS_INFO) == false){
        cout << "发送数据失败"<<endl;
        
    }
    if(client.recv_from_serv() == false){
        cout << "接收服务器发来的消息失败"<<endl;
    }
    NetPacketHeader *phead = (NetPacketHeader*)client.data_buffer;
    if(phead -> wOpcode == DEL_BOOKS_INFO_YES){
        cout << "删除成功"<< endl;
        return ;
    }
    if(phead -> wOpcode == SEA_BOOKS_INFO_NO){
        cout << "书籍不存在,没办法删除" << endl;
        return ;
    }
    if(phead -> wOpcode == DEL_BOOKS_INFO_NO){
        cout << "删除失败" << endl;
        return ;
    }
    
}
void del_book_name(TCPClient &client)
{
    bool non_stop = true;
    string temp_name1;
    string temp_name2;
    while(non_stop){
        cout << "请输入书籍名称:";
        cin >> temp_name1;
        cout << "请再次输入书籍名称确认:";
        cin >> temp_name2;
        if(temp_name1 == temp_name2){
            cout << "确认成功,正在删除"<<endl;
            break;
        }
        else{
            cout << "两次输入不一致,请重新输入"<<endl;
            continue;
        }
    }
    Json::Value book;
    string nu("");
    book["ISBN"] = nu.c_str();
    book["book_name"] = temp_name1.c_str();
    book["publish_house"] = nu.c_str();
    book["author"] = nu.c_str();
    book["count"] = nu.c_str();
    book["stat"] = nu.c_str();

    string out = book.toStyledString();
    memcpy(client.data_buffer,out.c_str(),out.size());
    if(client.send_to_serv(out.size(),DEL_BOOKS_INFO) == false){
        cout << "发送数据失败"<<endl;
        return ;
    }
    if(client.recv_from_serv() == false){
        cout << "接收服务器发来的消息失败"<<endl;
        return ;
    }
    NetPacketHeader *phead = (NetPacketHeader*)client.data_buffer;
    if(phead -> wOpcode == DEL_BOOKS_INFO_YES){
        cout << "删除成功"<<endl;
        return ;
    }
    if(phead -> wOpcode == SEA_BOOKS_INFO_NO){
        cout << "书籍不存在,没办法删除"<<endl;
        return ;
    }
    if(phead -> wOpcode == DEL_BOOKS_INFO_NO){
        cout << "删除失败"<<endl;
        return ;
    }

}
bool del_books_info(TCPClient &client){
    bool non_stop = true;
    string yes_or_no;
    Json::Value book;
    string ISBN,book_name,publish_house,author,count,stat;
    while(non_stop){
        cout << endl;
        cout << endl;
        cout << "是否开始删除程序[yes/no]:";
        cin >> yes_or_no;
        if(yes_or_no == "yes"){
            int choi;
            cout << "\t\t1.根据ISBN删除"<<endl;
            cout << "\t\t2.根据书籍名称删除"<<endl;
            //cout << "\t\t3.退出 "<< endl;
            cin >> choi;
            if((choi != 1) && (choi != 2) ){
                cout << "输入有误";
                continue;
            }
            switch(choi){
                case 1:del_isbn(client);break;
                case 2:del_book_name(client);break;
                //case 3:cout << "退出"<< endl;            }
            }
        }else{
            cout << "您选择的不是yes,现已退出删除程序"<<endl;
            non_stop = false;
        }
    }
}

bool chan_books_info(TCPClient &client){}
bool sea_books_info(TCPClient &client){}

void Admin_menu(TCPClient &client)
{
    int choice;
    bool non_stop = true;
    while (non_stop) {
        system("clear");
        cout << "\t\t欢迎进入管理员功能界面"<<endl;
        cout << "\t\t1.上线图书"<< endl;
        cout << "\t\t2.下线图书"<<endl;
        cout << "\t\t3.更改图书"<< endl;
        cout << "\t\t4.搜索图书"<<endl;
        cout << "\t\t5.退出" << endl;
        cin >> choice;

        switch (choice)
        {
            case 1:add_books_info(client); break;
            case 2:del_books_info(client); break;
            case 3:chan_books_info(client);break;
            case 4:sea_books_info(client);break;
            case 5: {cout<<"Bye"<<endl;     non_stop = false ;break;}
        }
        cout << "按任意见继续..."<< endl;
        getchar();
        getchar();
    }
}

void menu(TCPClient client) 
{
    int choice;
    
    cout<<"\t\t欢迎进入功能界面:"<<endl;
    cout<<"\t\t\t1.查询个人资料"<<endl;
    cout<<"\t\t\t2.聊天"<<endl;
    cout<<"\t\t\t3.退出"<<endl;
    cout<<"\n请输入选择序号:";

    while (1) {
        cin >> choice;

        switch (choice)
        {
            case 1: Personal_data(client); break;
            case 2:                        break;
            case 3: cout<<"Bye"<<endl;     exit(0);
        }
    }
}

void TCPClient::run(TCPClient& client)
{
    Login_Register(client);  //登录注册界面

}

int main(int argc ,char **argv)
{

    TCPClient client(argc,argv);


    client.run(client);

}
