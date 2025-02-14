#ifndef _MatchServer_H_
#define _MatchServer_H_

//
#include <iostream>
#include "servant/Application.h"
#include "OuterFactoryImp.h"

//
using namespace tars;

/**
 *
 **/
class MatchServer : public Application
{
public:
    /**
     *
     **/
    virtual ~MatchServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();

public:
    /*
    * 配置变更，重新加载配置
    */
    bool reloadSvrConfig(const string &command, const string &params, string &result);

    /**
     * 加载比赛场配置
    */
    bool reloadMatchConfig(const string &command, const string &params, string &result);

    /**
     * 清理比赛场
    */
    bool cleanMatchGame(const string &command, const string &params, string &result);

public:
    /**
    * 初始化外部接口对象
    **/
    int initOuterFactory();

    /**
    * 取外部接口对象
    **/
    OuterFactoryImpPtr getOuterFactoryPtr()
    {
        return _pOuter;
    }

private:
    OuterFactoryImpPtr _pOuter;  //外部接口对象
};

//
extern MatchServer g_app;

////////////////////////////////////////////
#endif
