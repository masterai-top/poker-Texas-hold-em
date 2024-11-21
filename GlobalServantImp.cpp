#include "GlobalServantImp.h"
#include "servant/Application.h"
#include "ServiceDefine.h"
#include "DBOperator.h"
#include "globe.h"
#include "LogComm.h"
#include "JFGameHttpProto.h"
#include "CommonStruct.h"
#include "CommonCode.h"
#include "XGameComm.pb.h"
#include "nickname.pb.h"
#include "Push.h"
#include "GlobalServer.h"
#include "WordFilter.h"
#include "RankBoard.pb.h"

//////////////////////////////////////////////////////

//排行榜上次刷新时间
static volatile int64_t m_LastUpdateRankingTime = 0L;
//当前生涯段位排行榜上次刷新时间
static volatile int64_t m_CurrGradeUpdateRankingTime = 0L;
//历史生涯段位排行榜上次刷新时间
static volatile int64_t m_LastGradeUpdateRankingTime = 0L;

//用户金币排行榜
static wbl::Mutex g_lockGoldRanking;
static std::vector<tagRankItemInfo> g_vGoldRanking;

//用户等级排行榜
static wbl::Mutex g_lockLevelRanking;
static std::vector<tagRankItemInfo> g_vLevelRanking;

//当前赛季排行榜
static wbl::Mutex g_lockWorldGradeRanking;
static std::vector<tagRankItemInfo> g_vWorldGradeRanking;
static std::map<int, std::vector<tagRankItemInfo>> g_vAreaGradeRanking;

//上个赛季排行榜
static wbl::Mutex g_lockWorldGradeRankLast;
static std::map<int, std::vector<tagRankItemInfo>> g_vAreaGradeRankLast;
static std::vector<tagRankItemInfo> g_vWorldGradeRankLast;

//用户AI场分数排行榜
static wbl::Mutex g_lockAIScoreRanking;
static std::vector<tagRankItemInfo> g_vAIRanking;

static string DatetimeToString(time_t time)
{
    tm *tm_ = localtime(&time);                 // 将time_t格式转换为tm结构体
    int year, month, day;                       // 定义时间的各个int临时变量。
    year = tm_->tm_year + 1900;                 // 临时变量，年，由于tm结构体存储的是从1900年开始的时间，所以临时变量int为tm_year加上1900。
    month = tm_->tm_mon + 1;                    // 临时变量，月，由于tm结构体的月份存储范围为0-11，所以临时变量int为tm_mon加上1。
    day = tm_->tm_mday;                         // 临时变量，日。

    string date = I2S(year) + "/" + I2S(month) + "/" + I2S(day);
    return date;                                // 返回转换日期时间后的string变量。
}

static string gettoday()
{
    return DatetimeToString(TC_TimeProvider::getInstance()->getNow());
}

GlobalServantImp::GlobalServantImp()
{

}

GlobalServantImp::~GlobalServantImp()
{

}

void GlobalServantImp::initialize()
{
    //initialize servant here:
    //
    initDBOperator();

    //
    initSensitiveWords();

    //...
    initializeTimer();
}

//////////////////////////////////////////////////////
void GlobalServantImp::destroy()
{
    //destroy servant here:
    //...
}

void GlobalServantImp::initializeTimer()
{
    m_threadTimer.initialize(this);
    m_threadTimer.start();
}

void GlobalServantImp::initDBOperator()
{
    const global::DBConf &dbConf = g_app.getOuterFactoryPtr()->getDBConfig();
    int iRet = _dbOperator.init(dbConf.Host, dbConf.user, dbConf.password, dbConf.dbname, dbConf.charset, dbConf.port);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "Init DBOperator failed, exit server." << endl;
        return;
    }

    iRet = _dbOperator.loadConfig();
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "load config failed, exit server." << endl;
        return;
    }
}

void GlobalServantImp::initSensitiveWords()
{
    g_app.getOuterFactoryPtr()->initWordFilter(_dbOperator.getSensitiveWordsConfig());
}


//http请求处理接口
tars::Int32 GlobalServantImp::doRequest(const vector<tars::Char> &reqBuf, const map<std::string, std::string> &extraInfo, vector<tars::Char> &rspBuf, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "recive request, reqBuf size : " << reqBuf.size() << endl;

    if (reqBuf.empty())
    {
        iRet = -1;
        return iRet;
    }

    THttpPackage thttpPack;
    if (!reqBuf.empty())
    {
        toObj(reqBuf, thttpPack);
    }

    if (thttpPack.vecData.empty())
    {
        iRet = -2;
        return iRet;
    }

    THttpPackage thttpPackRsp;
    thttpPackRsp.stUid.lUid = thttpPack.stUid.lUid;
    thttpPackRsp.stUid.sToken = thttpPack.stUid.sToken;
    thttpPackRsp.iVer = thttpPack.iVer;
    thttpPackRsp.iSeq = thttpPack.iSeq;
    tobuffer(thttpPackRsp, rspBuf);
    ROLLLOG_DEBUG << "response buff size: " << rspBuf.size() << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//tcp请求处理接口
tars::Int32 GlobalServantImp::onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current)
{
    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", addr : " << stClientParam.sAddr << endl;

    async_response_onRequest(current, 0);

    XGameComm::TPackage pkg;
    pbToObj(sMsgPack, pkg);

    if (pkg.vecmsghead_size() == 0)
    {
        ROLLLOG_DEBUG << "package empty." << endl;
        return -1;
    }

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", msg : " << logPb(pkg) << endl;

    for (int i = 0; i < pkg.vecmsghead_size(); ++i)
    {
        int64_t ms1 = TNOWMS;

        const auto &head = pkg.vecmsghead(i);
        switch(head.nmsgid())
        {
        // case XGameProto::ActionName::NICKNAME_RANDOM_NICKNAME:
        // {
        //     NicknameProto::ListNickNameRandomReq listNickNameRandomReq;
        //     pbToObj(pkg.vecmsgdata(i), listNickNameRandomReq);
        //     iRet = onRandomNickname(pkg, listNickNameRandomReq, sCurServrantAddr);
        //     break;
        // }
        case XGameProto::ActionName::QUERY_RANK_BOARD:
        {
            RankBoardProto::QueryRankInfoReq rankingReq;
            pbToObj(pkg.vecmsgdata(i), rankingReq);
            iRet = onQueryRankInfo(pkg, rankingReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::QUERY_FRIENDS_AIRANK_BOARD:
        {
            iRet = onQueryFriendAIRankInfo(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::GAME_RECORD_AI_BASE_INFO_QUERY:
        {
            iRet = onQueryAIBaseInfo(pkg, sCurServrantAddr, current);
        }
        default:
        {
            ROLLLOG_ERROR << "invalid msg id, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
            break;
        }
        }

        if (iRet != 0)
        {
            ROLLLOG_ERROR << "Process msg fail, uid: " << lUin << ", msg id: " << head.nmsgid()  << ", iRet: " << iRet << endl;
        }
        else
        {
            ROLLLOG_DEBUG << "Process msg succ, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
        }

        int64_t ms2 = TNOWMS;
        if ((ms2 - ms1) > COST_MS)
        {
            ROLLLOG_WARN << "@Performance, msgid:" << head.nmsgid() << ", costTime:" << (ms2 - ms1) << endl;
        }
    }

    __CATCH__
    return iRet;
}

// 获取随机昵称
// tars::Int32 GlobalServantImp::ListNickNameRandom(const nickname::ListNickNameRandomReq &req, nickname::ListNickNameRandomResp &resp, tars::TarsCurrentPtr current)
// {
//     int iRet = 0;

//     __TRY__

//     int nicknameCount = g_app.getOuterFactoryPtr()->getFetchNicknameNum();
//     if (req.count <= 0 || req.count > nicknameCount)
//         return -1;

//     int count = g_app.getOuterFactoryPtr()->getLoadNicknameNum();
//     iRet = _dbOperator.listNikckNameRandom(req, resp, count);
//     ROLLLOG_DEBUG << "ListNickNameRandom, req: " << printTars(req) << ", resp: " << printTars(resp) << endl;
//     __CATCH__
//     return iRet;
// }

// 变更昵称状态
// tars::Int32 GlobalServantImp::UpdateNickNameStatus(const nickname::UpdateNickNameStatusReq &req, nickname::UpdateNickNameStatusResp &resp, tars::TarsCurrentPtr current)
// {
//     int iRet = 0;

//     __TRY__

//     if (req.nickname.length() == 0)
//     {
//         ROLLLOG_ERROR << "parameter err, req: " << printTars(req) << ", iRet: -1" << endl;
//         return -1;
//     }

//     if (req.status != 0 && req.status != 1)
//     {
//         ROLLLOG_ERROR << "parameter err, req: " << printTars(req) << ", iRet: -2" << endl;
//         return -2;
//     }

//     iRet = _dbOperator.updateNickNameStatus(req, resp);
//     ROLLLOG_DEBUG << "UpdateNickNameStatus, req: " << printTars(req) << ", resp: " << printTars(resp) << ", iRet: " << iRet << endl;

//     __CATCH__
//     return iRet;
// }

//获取游戏状态
// int GlobalServantImp::onRandomNickname(const XGameComm::TPackage &pkg, const NicknameProto::ListNickNameRandomReq listNickNameRandomReq, const std::string &sCurServrantAddr)
// {
//     FUNC_ENTRY("");
//     int iRet = 0;

//     __TRY__

//     ROLLLOG_DEBUG << "onRandomNickname, req: " << logPb(listNickNameRandomReq) << endl;

//     NicknameProto::ListNickNameRandomResp listNickNameRandomResp;
//     int nicknameCount = g_app.getOuterFactoryPtr()->getFetchNicknameNum();
//     if (listNickNameRandomReq.count() <= 0 || listNickNameRandomReq.count() > nicknameCount)
//     {
//         listNickNameRandomResp.set_resultcode(XGameRetCode::ARG_INVALIDATE_ERROR);
//         toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::NICKNAME_RANDOM_NICKNAME, listNickNameRandomResp);
//         return -1;
//     }

//     //取昵称列表
//     nickname::ListNickNameRandomReq req;
//     nickname::ListNickNameRandomResp resp;
//     req.count = listNickNameRandomReq.count();
//     int count = g_app.getOuterFactoryPtr()->getLoadNicknameNum();
//     iRet = _dbOperator.listNikckNameRandom(req, resp, count);
//     ROLLLOG_DEBUG << "onRandomNickname, req: " << printTars(req) << ", resp: " << printTars(resp) << ", iRet: " << iRet << endl;
//     if (iRet < 0)
//     {
//         listNickNameRandomResp.set_resultcode(XGameRetCode::INNER_ERROR);
//     }
//     else
//     {
//         listNickNameRandomResp.set_resultcode(XGameRetCode::SUCCESS);
//         for (auto it = resp.nickname.begin(); it != resp.nickname.end(); ++it)
//         {
//             listNickNameRandomResp.add_nickname(*it);
//         }
//     }

//     toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::NICKNAME_RANDOM_NICKNAME, listNickNameRandomResp);

//     __CATCH__

//     FUNC_EXIT("", iRet);
//     return iRet;
// }

//发送消息到客户端
template<typename T>
int GlobalServantImp::toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t)
{
    XGameComm::TPackage rsp;
    rsp.set_iversion(tPackage.iversion());

    auto ptuid = rsp.mutable_stuid();
    ptuid->set_luid(tPackage.stuid().luid());
    rsp.set_igameid(tPackage.igameid());
    rsp.set_sroomid(tPackage.sroomid());
    rsp.set_iroomserverid(tPackage.iroomserverid());
    rsp.set_isequence(tPackage.isequence());
    rsp.set_iflag(tPackage.iflag());

    auto mh = rsp.add_vecmsghead();
    mh->set_nmsgid(actionName);
    mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_NICKNAME);
    rsp.add_vecmsgdata(pbToString(t));

    auto pPushPrx = Application::getCommunicator()->stringToProxy<JFGame::PushPrx>(sCurServrantAddr);
    if (pPushPrx)
    {
        ROLLLOG_DEBUG << "toclient pb: " << logPb(rsp) << ", t: " << logPb(t) << endl;
        pPushPrx->tars_hash(tPackage.stuid().luid())->async_doPushBuf(NULL, tPackage.stuid().luid(), pbToString(rsp));
    }
    else
    {
        ROLLLOG_ERROR << "pPushPrx is null: " << logPb(rsp) << ", t: " << logPb(t) << endl;
    }

    return 0;
}

//过滤文本
tars::Int32 GlobalServantImp::filterWords(const wordfilter::WordFilterReq &req, wordfilter::WordFilterResp &resp, tars::TarsCurrentPtr current)
{
    int iRet = 0;
    __TRY__

    resp.dstWords = req.words;

    //
    // int number = g_app.getOuterFactoryPtr()->utf8Count((unsigned char*)resp.dstWords.c_str(), resp.dstWords.length());
    // ROLLLOG_DEBUG << "number: " << number << endl;

    //去除特殊字符
    // bool bRet = g_app.getOuterFactoryPtr()->filterUtf8((unsigned char*)resp.dstWords.c_str(), resp.dstWords.length());
    // if(bRet)
    // {
    //  resp.resultCode = 1;
    //  return 1;
    // }

    //过滤敏感词
    bool bRet = g_app.getOuterFactoryPtr()->filter(resp.dstWords);
    if (bRet)
    {
        resp.resultCode = 2;
        return 2;
    }

    ROLLLOG_DEBUG << "filter words, req: " << printTars(req) << "resp: " << printTars(resp) << endl;

    __CATCH__
    return iRet;
}

// 存的时候gold * -1,因此取最小的15个即可
tars::Int32 GlobalServantImp::updateRankBoard(const XGame::UpdateRankBoardReq &req, XGame::UpdateRankBoardResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();

    if (req.type == 0)
    {
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_GOLD) + ":" + I2S(RANK_BOARD_GOLD);
        wdataReq.clusterInfo.frageFactor = RANK_BOARD_GOLD;
    }
    else
    {
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_LEVEL) + ":" + I2S(RANK_BOARD_LEVEL);
        wdataReq.clusterInfo.frageFactor = RANK_BOARD_LEVEL;
    }

    wdataReq.operateType = E_REDIS_INSERT;//TODO 可能是Write
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;

    dbagent::TField tfield;
    tfield.colArithType = E_NONE;
    tfield.colValue = I2S(req.uid);

    vector<dbagent::TField> fields;
    fields.push_back(tfield);
    if (req.type == 0)
        tfield.colValue = I2S(req.gold * -1);
    else
        tfield.colValue = I2S(req.level * -1);
    fields.push_back(tfield);
    wdataReq.fields = fields;

    dataproxy::TWriteDataRsp wdataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(req.uid)->redisWrite(wdataReq, wdataRsp);
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "updateRankBoard failed! iRet:" << iRet << endl;
        return -1;
    }

    auto iter = _uids.insert(req.uid);
    if (iter.second)
    {
        _rankBoardLen += 1;
        if (_rankBoardLen > MAX_RANK_BOARD_CAPACITY)
        {
            if (deleteNeedless(req.type) == 0)
            {
                ROLLLOG_DEBUG << "before delete sset:_rankBoardLen" << _rankBoardLen << endl;
                _rankBoardLen = g_app.getOuterFactoryPtr()->GetRankPlayerCount();
                ROLLLOG_DEBUG << "after delete sset:_rankBoardLen" << _rankBoardLen << endl;
            }
        }
    }

    ROLLLOG_DEBUG << "updateRankBoard succ, iRet:" << iRet << ",data:" << printTars(wdataRsp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//查询排行榜
tars::Int32 GlobalServantImp::queryRankInfo(const XGame::QueryRankInfoReq &req, XGame::QueryRankInfoResp &resp, tars::TarsCurrentPtr current)
{
    int iRet = 0;
    FUNC_ENTRY("");

    __TRY__

    resp.resultCode = 0;
    resp.rankingType = req.type;

    if (0 == req.type)//金币
    {
        wbl::MutexLocker lock(g_lockGoldRanking);
        int num = g_vGoldRanking.size();
        for (int i = 0; i < num; i++)
        {
            auto &data = g_vGoldRanking[i];
            XGame::RankItem ri;
            ri.uid = data.userid;
            ri.gold = data.gold;
            ri.level = data.level;
            ri.starNum = data.starNum;
            ri.danGrade = data.danGrade;
            resp.list.push_back(ri);
        }

        LOG_DEBUG << "g_vGoldRanking data, num: " << num << endl;
    }
    else if (1 == req.type)//等级
    {
        wbl::MutexLocker lock(g_lockLevelRanking);
        int num = g_vLevelRanking.size();
        for (int i = 0; i < num; i++)
        {
            auto &data = g_vLevelRanking[i];
            XGame::RankItem ri;
            ri.uid = data.userid;
            ri.gold = data.gold;
            ri.level = data.level;
            ri.starNum = data.starNum;
            ri.danGrade = data.danGrade;
            resp.list.push_back(ri);
        }

        LOG_DEBUG << "g_vLevelRanking data, num: " << num << endl;
    }
    else if (2 == req.type)//世界段位
    {
        int num = 0;
        int season_type = req.seasonType;
        if (season_type == 1) //当前赛季
        {
            wbl::MutexLocker lock(g_lockWorldGradeRanking);
            num = g_vWorldGradeRanking.size();
            for (int i = 0; i < num; i++)
            {
                auto &data = g_vWorldGradeRanking[i];
                XGame::RankItem ri;
                ri.uid = data.userid;
                ri.gold = data.gold;
                ri.level = data.level;
                ri.starNum = data.starNum;
                ri.danGrade = data.danGrade;
                resp.list.push_back(ri);
            }

            ROLLLOG_DEBUG << "g_vWorldGradeRanking data, num: " << num << ", req.seasonType: " << req.seasonType << endl;
        }
        else if (season_type == 2)  //上次赛季
        {
            wbl::MutexLocker lock(g_lockWorldGradeRankLast);
            num = g_vWorldGradeRankLast.size();
            for (int i = 0; i < num; i++)
            {
                auto &data = g_vWorldGradeRankLast[i];
                XGame::RankItem ri;
                ri.uid = data.userid;
                ri.gold = data.gold;
                ri.level = data.level;
                ri.starNum = data.starNum;
                ri.danGrade = data.danGrade;
                resp.list.push_back(ri);
            }

            ROLLLOG_DEBUG << "g_vWorldGradeRankLast data, num: " << num << ", req.seasonType: " << req.seasonType << endl;
        }
        else
        {
            ROLLLOG_ERROR << "req.type: " << req.type << ", req.seasonType: " << req.areaId << " is ERROR!" << endl;
        }
    }
    else if (3 == req.type)//区域段位
    {
        int num = 0;
        int area_id = req.areaId;
        int season_type = req.seasonType;
        if (season_type == 1)
        {
            wbl::MutexLocker lock(g_lockWorldGradeRanking);
            auto it = g_vAreaGradeRanking.find(area_id);
            if (it != g_vAreaGradeRanking.end())
            {
                num = it->second.size();
                for (auto itRank = it->second.begin(); itRank != it->second.end(); ++itRank)
                {
                    XGame::RankItem ri;
                    ri.uid = itRank->userid;
                    ri.gold = itRank->gold;
                    ri.level = itRank->level;
                    ri.starNum = itRank->starNum;
                    ri.danGrade = itRank->danGrade;
                    resp.list.push_back(ri);
                }

                ROLLLOG_DEBUG << "g_vAreaGradeRanking data, num: " << num << endl;
            }
            else
            {
                ROLLLOG_ERROR << "g_vAreaGradeRanking no find req.areaId: " << req.areaId << ", req.type: " << req.type << endl;
            }
        }
        else if (season_type == 2)
        {
            wbl::MutexLocker lock(g_lockWorldGradeRankLast);
            auto it = g_vAreaGradeRankLast.find(area_id);
            if (it != g_vAreaGradeRankLast.end())
            {
                num = it->second.size();
                for (auto itRank = it->second.begin(); itRank != it->second.end(); ++itRank)
                {
                    XGame::RankItem ri;
                    ri.uid = itRank->userid;
                    ri.gold = itRank->gold;
                    ri.level = itRank->level;
                    ri.starNum = itRank->starNum;
                    ri.danGrade = itRank->danGrade;
                    resp.list.push_back(ri);
                }

                ROLLLOG_DEBUG << "g_vAreaGradeRankLast data, num: " << num << endl;
            }
            else
            {
                ROLLLOG_ERROR << "g_vAreaGradeRankLast no find req.areaId: " << req.areaId << ", req.type: " << req.type << endl;
            }
        }
        else
        {
            ROLLLOG_ERROR << "req.type: " << req.type << "req.seasonType: " << req.areaId << " is ERROR!" << endl;
        }
    }
    else if(4 == req.type)
    {
        wbl::MutexLocker lock(g_lockAIScoreRanking);
        int num = g_vAIRanking.size();
        for (int i = 0; i < num; i++)
        {
            auto &data = g_vAIRanking[i];
            XGame::RankItem ri;
            ri.uid = data.userid;
            ri.gold = data.gold;
            ri.level = data.level;
            ri.starNum = data.starNum;
            ri.danGrade = data.danGrade;
            resp.list.push_back(ri);
        }

        LOG_DEBUG << "g_vAIRanking data, num: " << num << "resp: " << printTars(resp) << endl;
    }
    // dataproxy::TReadDataReq dataReq;
    // dataReq.resetDefautlt();
    // if (req.type == 0)
    // {
    //     dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_GOLD) + ":" + I2S(RANK_BOARD_GOLD);
    //     dataReq.clusterInfo.frageFactor = RANK_BOARD_GOLD;
    // }
    // else
    // {
    //     dataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_LEVEL) + ":" + I2S(RANK_BOARD_LEVEL);
    //     dataReq.clusterInfo.frageFactor = RANK_BOARD_LEVEL;
    // }
    // dataReq.operateType = E_REDIS_READ;
    // dataReq.clusterInfo.busiType = E_PROPERTY;
    // dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    // dataReq.paraExt.resetDefautlt();
    // dataReq.paraExt.start = 0;
    // dataReq.paraExt.end = g_app.getOuterFactoryPtr()->GetRankPlayerCount();

    // TReadDataRsp dataRsp;
    // iRet = g_app.getOuterFactoryPtr()->getDataProxyServantPrx()->read(dataReq, dataRsp);
    // if (iRet != 0 || dataRsp.iResult != 0)
    // {
    //     ROLLLOG_ERROR << "get rank info err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
    //     resp.resultCode = -1;
    //     return -1;
    // }

    // int max_len = g_app.getOuterFactoryPtr()->GetRankPlayerCount();
    // resp.resultCode = 0;
    // for (auto row_ptr = dataRsp.fields.begin(); row_ptr != dataRsp.fields.end(); ++row_ptr)
    // {
    //     tars::Int64 member = S2L(row_ptr->front().colValue);
    //     tars::Int64 score = S2L(row_ptr->back().colValue);

    //     XGame::RankItem item;
    //     item.uid = member;
    //     if (req.type == 0)
    //         item.gold = score * -1;
    //     else
    //         item.level = score * -1;

    //     resp.list.push_back(item);
    //     if (resp.list.size() == (size_t)max_len)
    //     {
    //         break;
    //     }
    // }

    // ROLLLOG_DEBUG << "get rank info, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;

    __CATCH__

    FUNC_EXIT("", iRet);
    return iRet;
}

//
tars::Int32 GlobalServantImp::deleteNeedless(const int type)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    dataproxy::TWriteDataReq wdataReq;
    wdataReq.resetDefautlt();
    if (type == 0)
    {
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_GOLD) + ":" + I2S(RANK_BOARD_GOLD);
        wdataReq.clusterInfo.frageFactor = RANK_BOARD_GOLD;
    }
    else
    {
        wdataReq.keyName = I2S(E_REDIS_TYPE_SORT_SET) + ":" + I2S(RANK_BOARD_LEVEL) + ":" + I2S(RANK_BOARD_LEVEL);
        wdataReq.clusterInfo.frageFactor = RANK_BOARD_LEVEL;
    }

    wdataReq.operateType = E_REDIS_DELETE;//TODO 可能是Write
    wdataReq.clusterInfo.resetDefautlt();
    wdataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    wdataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    wdataReq.paraExt.resetDefautlt();
    wdataReq.paraExt.subOperateType = E_REDIS_SORTSET_REM_RANGE_BY_RANK;
    wdataReq.paraExt.start = g_app.getOuterFactoryPtr()->GetRankPlayerCount();
    wdataReq.paraExt.end = -1;

    dataproxy::TWriteDataRsp wdataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(type)->redisWrite(wdataReq, wdataRsp);
    if (iRet != 0 || wdataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "delete needless rank info err, iRet: " << iRet << ", iResult: " << wdataRsp.iResult << ", type=" << type << endl;
        return -1;
    }

    ROLLLOG_DEBUG << "delete needless rank info, iRet: " << iRet << ", dataRsp: " << printTars(wdataRsp) <<  ", type=" << type << endl;
    __CATCH__

    FUNC_EXIT("", iRet);
    return iRet;
}

//查询AI场基础信息
tars::Int32 GlobalServantImp::onQueryAIBaseInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__
    RankBoardProto::QueryAIBaseInfoResp resp;
    resp.set_uid(pkg.stuid().luid());

    int hdcount = 0;
    long score = 0;
    long aiwinscore = 0;
    int userwincount = 0;
    int usercount = 0;
    std::string userstr = L2S(pkg.stuid().luid());
    std::string today = gettoday();

    _dbOperator.loadAIUserScore(userstr, hdcount, score);
    _dbOperator.loadAIWinScore(today, aiwinscore);
    int handcount = g_app.getOuterFactoryPtr()->getAIMinHandCount();
    _dbOperator.loadAiWinInfo(userwincount, usercount, handcount);

    resp.set_hdcount(hdcount);
    resp.set_gold(score);
    resp.set_aigold(aiwinscore);
    resp.set_userwincount(userwincount);
    resp.set_usercount(usercount);
    ROLLLOG_ERROR << "onQueryAIBaseInfo resp: " << logPb(resp) << endl;
    toClientPb(pkg, sCurServrantAddr, XGameProto::GAME_RECORD_AI_BASE_INFO_QUERY, resp);
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//查询好友AI场分数排行榜
tars::Int32 GlobalServantImp::onQueryFriendAIRankInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__
    RankBoardProto::QueryFriendAIRankInfoResp resp;

    Friends::QueryFriendListReq queryReq;
    queryReq.uid = pkg.stuid().luid();
    Friends::QueryFriendListResp queryResp;
    iRet = g_app.getOuterFactoryPtr()->getSocialServantPrx(queryReq.uid)->QueryFriends(queryReq, queryResp);
    if ((iRet != 0) || (queryResp.resultCode != 0))
    {
        ROLLLOG_ERROR << "QueryFriends failed, iRet:" << iRet << ", queryResp.resultCode:" << queryResp.resultCode << endl;
        resp.set_resultcode(-1);
        toClientPb(pkg, sCurServrantAddr, XGameProto::QUERY_FRIENDS_AIRANK_BOARD, resp);
        return -1;
    }

    std::string uidList;
    for (auto it = queryResp.FriendList.begin(); it != queryResp.FriendList.end(); ++it)
    {
        if (it == queryResp.FriendList.begin())
        {
            uidList += "(";
        }
        else
        {
            uidList += ",";
        }

        uidList += L2S(*it);
    }

    if (!uidList.empty())
    {
        std::string uidStr = "," + L2S(queryReq.uid) + ")";
        uidList += uidStr;
        std::vector<tagRankItemInfo> vFriendAIRanking;
        std::vector<int> handcount;
        handcount = g_app.getOuterFactoryPtr()->getAIRankMinHandCount();
        _dbOperator.loadFriendAIScoreRanking(uidList, vFriendAIRanking, handcount[0], handcount[1]);
        for(auto &i : vFriendAIRanking)
        {
            auto item = resp.add_list();
            item->set_uid(i.userid);
            item->set_gold(i.gold);
            item->set_level(i.level);
            item->set_starnum(i.starNum);
        }
    }

    resp.set_resultcode(0);
    ROLLLOG_ERROR << "onQueryFriendAIRankInfo resp: " << logPb(resp) << endl;
    toClientPb(pkg, sCurServrantAddr, XGameProto::QUERY_FRIENDS_AIRANK_BOARD, resp);
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//检查排行榜请求
tars::Int32 GlobalServantImp::onQueryRankInfo(const XGameComm::TPackage &pkg, const RankBoardProto::QueryRankInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    XGame::QueryRankInfoReq reqRanking;
    reqRanking.type = req.type();
    reqRanking.areaId = req.areaid();
    reqRanking.seasonType = req.seasontype();

    XGame::QueryRankInfoResp resp;
    iRet = queryRankInfo(reqRanking, resp, current);
    RankBoardProto::QueryRankInfoResp response;
    if (iRet != 0 || resp.resultCode != 0)
    {
        ROLLLOG_DEBUG << "query info error:" << endl;
        response.set_resultcode(-1);
        response.set_rankingtype(resp.rankingType);
        toClientPb(pkg, sCurServrantAddr, XGameProto::QUERY_RANK_BOARD, response);
        return -1;
    }

    response.set_resultcode(0);
    response.set_rankingtype(resp.rankingType);
    response.set_seasontype(req.seasontype());
    for (auto it = resp.list.begin(); it != resp.list.end(); ++it)
    {
        auto pItem = response.add_list();
        pItem->set_uid(it->uid);
        pItem->set_gold(it->gold);
        pItem->set_level(it->level);
        pItem->set_starnum(it->starNum);
        pItem->set_dangrade(it->danGrade);
    }

    toClientPb(pkg, sCurServrantAddr, XGameProto::QUERY_RANK_BOARD, response);
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//获取取区域标识
tars::Int32 GlobalServantImp::getAreaID(const iptocountry::AreaIDReq &req, iptocountry::AreadIDResp &rsp, tars::TarsCurrentPtr current)
{
    int iRet = 0;
    __TRY__

    if (req.ip.empty())
    {
        rsp.areaID = E_AREA_ID_UNKNOWN;
        return -1;
    }

    string countryCode = ""; //国家码
    iRet = g_app.getOuterFactoryPtr()->lookupAreaID(req.ip, countryCode);
    if (iRet != 0)
    {
        rsp.areaID = E_AREA_ID_UNKNOWN;
        return -2;
    }

    const map<string, int> &areaID = g_app.getOuterFactoryPtr()->getAreaID();
    auto iter = areaID.find(countryCode);
    if (iter == areaID.end())
    {
        rsp.areaID = E_AREA_ID_UNKNOWN;
        return -3;
    }

    rsp.areaID = iter->second;
    __CATCH__
    return iRet;
}

// 刷新赛季排行榜数据
tars::Int32 GlobalServantImp::RefreshSeasonRankInfo(tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;

    __TRY__
    ROLLLOG_DEBUG << "Refresh grade rankings(1)" << endl;
    wbl::MutexLocker lock(g_lockWorldGradeRanking);
    g_vWorldGradeRanking.clear();
    _dbOperator.loadWorldGradeRanking(g_vWorldGradeRanking);
    g_vAreaGradeRanking.clear();
    _dbOperator.loadAreaGradeRanking(g_vAreaGradeRanking);
    __CATCH__

    __TRY__
    ROLLLOG_DEBUG << "Refresh grade rankings(2)" << endl;
    wbl::MutexLocker lock(g_lockWorldGradeRankLast);
    g_vWorldGradeRankLast.clear();
    _dbOperator.loadWorldGradeRankLast(g_vWorldGradeRankLast);
    g_vAreaGradeRankLast.clear();
    _dbOperator.loadAreaGradeRankLast(g_vAreaGradeRankLast);
    __CATCH__

    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 GlobalServantImp::doCustomMessage(bool bExpectIdle)
{
    if (bExpectIdle)
    {
        static uint32_t ii = 0;
        static uint32_t hz = 60 * 60;
        if (0 == ((ii++) % hz))
        {
            uint32_t now = TNOW;

            static uint32_t dayCounter = 10 * 60;
            if ((0 == m_LastUpdateRankingTime)  || (now / dayCounter) != (m_LastUpdateRankingTime / dayCounter))
            {
                ROLLLOG_DEBUG << "Update grade rankings(1)" << endl;
                m_LastUpdateRankingTime = now;

                __TRY__
                wbl::MutexLocker lock(g_lockGoldRanking);
                g_vGoldRanking.clear();
                _dbOperator.loadUserGlodRanking(g_vGoldRanking);
                __CATCH__

                __TRY__
                wbl::MutexLocker lock(g_lockLevelRanking);
                g_vLevelRanking.clear();
                _dbOperator.loadUserLevelRanking(g_vLevelRanking);
                __CATCH__

                __TRY__
                wbl::MutexLocker lock(g_lockAIScoreRanking);
                g_vAIRanking.clear();
                std::vector<int> handcount;
                handcount = g_app.getOuterFactoryPtr()->getAIRankMinHandCount();
                _dbOperator.loadAIScoreRanking(g_vAIRanking, handcount[0], handcount[1]);
                __CATCH__
            }

            static uint32_t dayCounter2 = 20 * 60;
            if ((0 == m_CurrGradeUpdateRankingTime) || ((now / dayCounter2) != (m_CurrGradeUpdateRankingTime / dayCounter2)))
            {
                ROLLLOG_DEBUG << "Update grade rankings(2)" << endl;
                m_CurrGradeUpdateRankingTime = now;

                __TRY__
                wbl::MutexLocker lock(g_lockWorldGradeRanking);
                g_vWorldGradeRanking.clear();
                _dbOperator.loadWorldGradeRanking(g_vWorldGradeRanking);
                g_vAreaGradeRanking.clear();
                _dbOperator.loadAreaGradeRanking(g_vAreaGradeRanking);
                __CATCH__
            }

            static uint32_t dayCounter3 = 60 * 60;
            if ((0 == m_LastGradeUpdateRankingTime) || ((now / dayCounter3) != (m_LastGradeUpdateRankingTime / dayCounter3)))
            {
                ROLLLOG_DEBUG << "Update grade rankings(3)" << endl;
                m_LastGradeUpdateRankingTime = now;

                __TRY__
                wbl::MutexLocker lock(g_lockWorldGradeRankLast);
                g_vWorldGradeRankLast.clear();
                _dbOperator.loadWorldGradeRankLast(g_vWorldGradeRankLast);
                g_vAreaGradeRankLast.clear();
                _dbOperator.loadAreaGradeRankLast(g_vAreaGradeRankLast);
                __CATCH__
            }
        }
    }

    return 0;
}
