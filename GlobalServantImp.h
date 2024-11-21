#ifndef _GlobalServantImp_H_
#define _GlobalServantImp_H_

#include "servant/Application.h"
#include "GlobalServant.h"
#include "XGameComm.pb.h"
#include "CommonCode.pb.h"
#include "CommonStruct.pb.h"
#include "config.pb.h"
#include "nickname.pb.h"
#include "RankBoard.pb.h"
#include "globe.h"
#include "LogComm.h"
#include "JFGameHttpProto.h"
#include "Push.h"
#include "TimerThread.h"
#include "DBOperator.h"
#include "OuterFactoryImp.h"

//
using namespace std;
using namespace JFGame;
using namespace JFGameHttpProto;
using namespace nickname;
using namespace global;
using namespace iptocountry;

/**
 *
 *服务接口
 */
class GlobalServantImp : public GlobalServant
{
public:
    /**
     *
     */
    GlobalServantImp();

    /**
     *
     */
    virtual ~GlobalServantImp();

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
     *
     */
    void initializeTimer();

    /**
     *
     */
    void initDBOperator();

    /**
     *
     */
    void initSensitiveWords();

public:
    //HTTP请求处理接口
    virtual tars::Int32 doRequest(const vector<tars::Char> &reqBuf, const map<std::string, std::string> &extraInfo, vector<tars::Char> &rspBuf, tars::TarsCurrentPtr current);
    //TCP请求处理接口
    virtual tars::Int32 onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current);

public:
    // 获取随机昵称
    // virtual tars::Int32 ListNickNameRandom(const nickname::ListNickNameRandomReq &req, nickname::ListNickNameRandomResp &resp, tars::TarsCurrentPtr current);
    // 变更昵称状态
    // virtual tars::Int32 UpdateNickNameStatus(const nickname::UpdateNickNameStatusReq &req, nickname::UpdateNickNameStatusResp &resp, tars::TarsCurrentPtr current);
    //取区域信息
    virtual tars::Int32 getAreaID(const iptocountry::AreaIDReq &req, iptocountry::AreadIDResp &rsp, tars::TarsCurrentPtr current);
    //过滤文本
    virtual tars::Int32 filterWords(const wordfilter::WordFilterReq &req, wordfilter::WordFilterResp &resp, tars::TarsCurrentPtr current);
    //更新排行榜
    virtual tars::Int32 updateRankBoard(const XGame::UpdateRankBoardReq &req, XGame::UpdateRankBoardResp &resp,   tars::TarsCurrentPtr current);
    //查询排行榜
    virtual tars::Int32 queryRankInfo(const XGame::QueryRankInfoReq &req, XGame::QueryRankInfoResp &resp, tars::TarsCurrentPtr current);
    // 刷新赛季排行榜数据
    virtual tars::Int32 RefreshSeasonRankInfo(tars::TarsCurrentPtr current);

public:
    //时钟周期回调
    virtual tars::Int32 doCustomMessage(bool bExpectIdle = false);
    //获取随机昵称
    // int onRandomNickname(const XGameComm::TPackage &pkg, const NicknameProto::ListNickNameRandomReq listNickNameRandomReq, const std::string &sCurServrantAddr);

private:
    //发送消息到客户端
    template<typename T>
    int toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t);

private:
    //删除相关
    tars::Int32 deleteNeedless(const int type);
    //查询排行榜请求
    tars::Int32 onQueryRankInfo(const XGameComm::TPackage &pkg, const RankBoardProto::QueryRankInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    //查询好友AI场分数排行榜
    tars::Int32 onQueryFriendAIRankInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    //查询AI场基础信息
    tars::Int32 onQueryAIBaseInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
private:
    //数据库连接对象
    CDBOperator _dbOperator;
    //排行榜数据长度
    tars::Int32 _rankBoardLen = 0;
    //排行榜数据容量
    const int MAX_RANK_BOARD_CAPACITY = 1024;
    //用户标识集合
    std::set<tars::Int64> _uids;
    //定时器线程
    TimerThread m_threadTimer;

    // public:
    //     //排行榜上次刷新时间
    //     int64_t m_LastUpdateRankingTime;
    //     //当前生涯段位排行榜上次刷新时间
    //     int64_t m_CurrGradeUpdateRankingTime;
    //     //历史生涯段位排行榜上次刷新时间
    //     int64_t m_LastGradeUpdateRankingTime;
    //     //金币排行榜
    //     std::vector<tagRankItemInfo> m_vGoldRanking;
    //     //等级排行榜
    //     std::vector<tagRankItemInfo> m_vLevelRanking;
    //     //当前赛季世界段位排行榜
    //     std::vector<tagRankItemInfo> m_vWorldGradeRanking;
    //     //上次赛季世界段位排行榜
    //     std::vector<tagRankItemInfo> m_vWorldGradeRankLast;
    //     //当前赛季区域段位排行榜
    //     std::map<int, std::vector<tagRankItemInfo>> m_vAreaGradeRanking;
    //     //上次赛季区域段位排行榜
    //     std::map<int, std::vector<tagRankItemInfo>> m_vAreaGradeRankLast;

    // private:
    //     //金币排行榜读写锁
    //     wbl::ReadWriteLocker m_lockGoldRanking;
    //     //等级排行榜读写锁
    //     wbl::ReadWriteLocker m_lockLevelRanking;
    //     //当前赛季世界段位排行榜读写锁
    //     wbl::ReadWriteLocker m_lockWorldGradeRanking;
    //     //上次赛季世界段位排行榜读写锁
    //     wbl::ReadWriteLocker m_lockWorldGradeRankLast;
};

/////////////////////////////////////////////////////
#endif
