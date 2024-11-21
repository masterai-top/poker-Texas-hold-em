#ifndef __CONFIG_DEFINE_H__
#define __CONFIG_DEFINE_H__

//
#include <string>
#include <map>

namespace global
{
    //数据源配置
    typedef struct _TDBConf
    {
        string Domain;        //域名
        string Host;          //主机
        string port;          //端口
        string user;          //用户名
        string password;      //密码
        string charset;       //编码格式
        string dbname;        //数据库
    } DBConf;
};

#endif



