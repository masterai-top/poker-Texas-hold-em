#include <sstream>
#include "OuterFactoryImp.h"
#include "LogComm.h"
#include "OrderServer.h"
#include "util/tc_hash_fun.h"

using namespace wbl;

namespace
{
    template<typename T>
    void mapInsert(T &m, const typename T::key_type &key, const typename T::mapped_type &value)
    {
        m.insert(make_pair(key, value));
    }

    template<typename T>
    std::string mToString(const T &param)
    {
        ostringstream os;
        os << param;
        return os.str();
    }

    template<>
    std::string mToString<std::string>(const std::string &param)
    {
        ostringstream os;
        os << "\"" << param << "\"";
        return os.str();
    }

    template<typename Key, typename Value>
    std::string mToString(const map<Key, Value> &param)
    {
        ostringstream os;
        os << "{";
        size_t len = param.size();
        size_t idx = 0;
        for (auto &elem : param)
        {
            os << "\"" << elem.first << "\":" << mToString(elem.second);
            if (idx != len - 1)
            {
                os << ",";
            }
            idx += 1;
        }
        os << "}";
        return os.str();
    }
}

/**
 *
*/
OuterFactoryImp::OuterFactoryImp() : _pFileConf(NULL)
{
    createAllObject();
}

/**
 *
*/
OuterFactoryImp::~OuterFactoryImp()
{
    deleteAllObject();
}

void OuterFactoryImp::deleteAllObject()
{
    if (_pFileConf)
    {
        delete _pFileConf;
        _pFileConf = NULL;
    }
}

void OuterFactoryImp::createAllObject()
{
    try
    {
        //
        deleteAllObject();

        //本地配置文件
        _pFileConf = new tars::TC_Config();
        if (!_pFileConf)
        {
            ROLLLOG_ERROR << "create config parser fail, ptr null." << endl;
            terminate();
        }

        //tars代理Factory,访问其他tars接口时使用
        _pProxyFactory = new OuterProxyFactory();
        if ((long int)NULL == _pProxyFactory)
        {
            ROLLLOG_ERROR << "create outer proxy factory fail, ptr null." << endl;
            terminate();
        }

        FDLOG_RECHARGE_LOG_FORMAT;
        LOG_DEBUG << "init proxy factory succ." << endl;

        //读取所有配置
        load();
    }
    catch (TC_Exception &ex)
    {
        LOG->error() << ex.what() << endl;
    }
    catch (exception &e)
    {
        LOG->error() << e.what() << endl;
    }
    catch (...)
    {
        LOG->error() << "unknown exception." << endl;
    }

    return;
}

//读取所有配置
void OuterFactoryImp::load()
{
    __TRY__

    //拉取远程配置
    g_app.addConfig(ServerConfig::ServerName + ".conf");

    wbl::WriteLocker lock(m_rwlock);

    _pFileConf->parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    LOG_DEBUG << "init config file succ:" << ServerConfig::BasePath + ServerConfig::ServerName + ".conf" << endl;

    //代理配置
    readPrxConfig();
    printPrxConfig();
    __CATCH__
}

//代理配置
void OuterFactoryImp::readPrxConfig()
{
    _urls.clear();

    // TODO 这里可以从配置文件读取
    mapInsert(_urls, order::E_CHANNEL_GOOGLEPLAY,   std::map<order::Eun_Order_Env, std::string>());
    mapInsert(_urls, order::E_CHANNEL_IOS,          std::map<order::Eun_Order_Env, std::string>());

    //配置服务
    _DBAgentServantObj = (*_pFileConf).get("/Main/Interface/DBAgentServer<ProxyObj>", "");
    _HallServantObj = (*_pFileConf).get("/Main/Interface/HallServer<ProxyObj>", "");
    _ActivityServantObj = (*_pFileConf).get("/Main/Interface/ActivityServer<ProxyObj>", "");
    _Log2DBServantObj = (*_pFileConf).get("/Main/Interface/Log2DBServer<ProxyObj>", "");

    std::string url;
    url = (*_pFileConf).get("/Main/url/GooglePlay<release>", "");
    mapInsert(_urls.at(order::E_CHANNEL_GOOGLEPLAY), order::E_ORDER_ENV_RELEASE, url);

    url = (*_pFileConf).get("/Main/url/GooglePlay<debug>", "");
    mapInsert(_urls.at(order::E_CHANNEL_GOOGLEPLAY), order::E_ORDER_ENV_DEBUG, url);

    url = (*_pFileConf).get("/Main/url/IOS<release>", "");
    mapInsert(_urls.at(order::E_CHANNEL_IOS), order::E_ORDER_ENV_RELEASE, url);

    url = (*_pFileConf).get("/Main/url/IOS<debug>", "");
    mapInsert(_urls.at(order::E_CHANNEL_IOS), order::E_ORDER_ENV_DEBUG, url);

    _ConfigServantObj = (*_pFileConf).get("/Main/Interface/ConfigServer<ProxyObj>", "");

    mallConfigRaw.clear();
    getConfigServantPrx()->ListMallConfigRaw(mallConfigRaw);

    activityConfigRaw.clear();
    getConfigServantPrx()->ListGoldPigConfigRaw(activityConfigRaw);
}

//打印代理配置
void OuterFactoryImp::printPrxConfig()
{
    FDLOG_CONFIG_INFO << "_DBAgentServantObj ProxyObj:" << _DBAgentServantObj << endl;
    FDLOG_CONFIG_INFO << "_HallServantObj ProxyObj:" << _HallServantObj << endl;
    FDLOG_CONFIG_INFO << "_ActivityServantObj ProxyObj:" << _ActivityServantObj << endl;
    FDLOG_CONFIG_INFO << "URLS:" << mToString(_urls) << endl;
    FDLOG_CONFIG_INFO << "_ConfigServantObj ProxyObj:" << _ConfigServantObj << endl;
    FDLOG_CONFIG_INFO << "_Log2DBServantObj ProxyObj : " << _Log2DBServantObj << endl;
}

const config::ConfigServantPrx OuterFactoryImp::getConfigServantPrx()
{
    if (!_ConfigServantPrx)
    {
        _ConfigServantPrx = Application::getCommunicator()->stringToProxy<config::ConfigServantPrx>(_ConfigServantObj);
        LOG_DEBUG << "Init _ConfigServantObj succ, _ConfigServantObj: " << _ConfigServantObj << endl;
    }

    return _ConfigServantPrx;
}

// 金猪代理
const activity::ActivityServantPrx OuterFactoryImp::getActivityServantPrx(const long uid)
{
    if (!_ActivityServerPrx)
    {
        _ActivityServerPrx = Application::getCommunicator()->stringToProxy<activity::ActivityServantPrx>(_ActivityServantObj);
        ROLLLOG_DEBUG << "Init _ActivityServantObj succ, _ActivityServantObj:" << _ActivityServantObj << endl;
    }

    if (_ActivityServerPrx)
    {
        return _ActivityServerPrx->tars_hash(uid);
    }

    return NULL;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const long uid)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj:" << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(uid);
    }

    return NULL;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const string key)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj:" << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}

//
const hall::HallServantPrx OuterFactoryImp::getHallServantPrx(const long uid)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<hall::HallServantPrx>(_HallServantObj);
        ROLLLOG_DEBUG << "Init _HallServantObj succ, _HallServantObj:" << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(uid);
    }

    return NULL;
}

//
const hall::HallServantPrx OuterFactoryImp::getHallServantPrx(const string key)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<hall::HallServantPrx>(_HallServantObj);
        ROLLLOG_DEBUG << "Init _HallServantObj succ, _HallServantObj:" << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}

//日志入库服务代理
const DaqiGame::Log2DBServantPrx OuterFactoryImp::getLog2DBServantPrx(const long uid)
{
    if (!_Log2DBServerPrx)
    {
        _Log2DBServerPrx = Application::getCommunicator()->stringToProxy<DaqiGame::Log2DBServantPrx>(_Log2DBServantObj);
        ROLLLOG_DEBUG << "Init _Log2DBServantObj succ, _Log2DBServantObj : " << _Log2DBServantObj << endl;
    }

    if (_Log2DBServerPrx)
    {
        return _Log2DBServerPrx->tars_hash(uid);
    }

    return NULL;
}

//日志入库
void OuterFactoryImp::asyncLog2DB(const int64_t uid, const DaqiGame::TLog2DBReq &req)
{
    getLog2DBServantPrx(uid)->async_log2db(NULL, req);
}

//格式化时间
string OuterFactoryImp::GetTimeFormat()
{
    string sFormat("%Y-%m-%d %H:%M:%S");
    time_t t = time(NULL);
    auto pTm = localtime(&t);
    if (!pTm)
        return "";

    char sTimeString[255] = "\0";
    strftime(sTimeString, sizeof(sTimeString), sFormat.c_str(), pTm);
    return string(sTimeString);
}

//获得时间秒数
int OuterFactoryImp::GetTimeTick(const string &str)
{
    if (str.empty())
        return 0;

    struct tm tm_time;
    string sFormat("%Y-%m-%d %H:%M:%S");
    strptime(str.c_str(), sFormat.c_str(), &tm_time);
    return mktime(&tm_time);
}

//拆分字符串成整形
int OuterFactoryImp::splitInt(string szSrc, vector<int> &vecInt)
{
    split_int(szSrc, "[ \t]*\\|[ \t]*", vecInt);
    return 0;
}

//拆分字符串
vector<std::string> OuterFactoryImp::split(const string &str, const string &pattern)
{
    return SEPSTR(str, pattern);
}

////////////////////////////////////////////////////////////////////////////////


