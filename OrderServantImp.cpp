#include "OrderServantImp.h"
#include "servant/Application.h"
#include "uuid.h"
#include "DataProxyProto.h"
#include "ServiceUtil.h"
#include "LogComm.h"
#include "globe.h"
#include "OrderServer.h"
#include "util/tc_hash_fun.h"
#include "Request.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "HallServant.h"
#include <jwt-cpp/jwt.h>
#include <iostream>
#include <chrono>
#include "LogDefine.h"

//
using namespace std;

//
using namespace dataproxy;
using namespace rapidjson;

#define __NASH_RELEASE__
//#define __USE_PROXY__
#ifdef __NASH_RELEASE__
const string private_key = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDjRaXIDDIT2u5x\n+gsg9A0vxZxZaaQHywzXon9XDRL2mQI/79ii/V7nVccFs9i7IgrCMMl2SoaoNpjT\nGFdMnP2LHRUg8+Ccc+A+ptX0w1d7Jh927BftuG/mLLCPo9Vk+mWBmiwW/GduKcd8\nlJ3iyKuPHFayLb69wU5H8J16yxeSw8oMakiYG8R38VZd3BBACOXcHbTMk4wPkCRj\nvU1My4M+X+y8oGf5/1zHWna3f130E4T6iL2c4Z5lhPnQeip/RZF0fHeOAcGBeJB9\n9hh+Hngf9h6K3acbdDo0P0vVenJis2yXKZB7jI+5ZxHc8mYoC2XfmLMn7+mYs6mV\nLMAE4cQZAgMBAAECggEAFyGiVSlKmoM8pByZL94vDmbsOhx6rbFgqfzXvZYu0Srz\nE+tXLwWfOGfeVQoAO9Gj6y5YfuHocSv3Y3rVfc53rsZDvYIc4QeZMf88hQgxue7A\nIpQ2fkM4nmJ7iO8Zm8ixdp6osKE5bLizXU5qAGhAr9ToTQPDZ2BKlZ9bg+EyUcVL\nBHbQEc+t7kXZMRiBtGVZHc4SRdi1FBSfL88dr205edC8IV+Ei9ccsNR34EQPqD7E\n1+0khdgkh5UNAYRH0lir11s45+6pz/IRz3w05EPGnQmwfLne1gatY2U5AYbUOZvD\nL5bR7jYP0MYLTYtKBHps5Uz4xY7u+QK4c6yfnwLh4QKBgQD52o19dqyC436S3VxF\nLKBMsD2pNm3kefql+yaqKr1Ylbll8qKyi0AzAEf3RQLAxlgXecC7SYWskK84M/6g\nYVzdtvFnWJT7i9mb+Z6lWzyBgaLC42oONLVIRYm2GFpEXnT6H5243Cep7Uzedm2J\nHRbsgZzoidEceskbtAk1PSzGaQKBgQDo3OMxI2HkD48ArkmqtODPx4P4XJG28bcu\nPJgToYRF9tSuBx02MPWfURGbchEnme0p/HqA/0rZ+sUYpLuU6m7UsCMEMKTdbJxw\n6Xh4Z6ljIjlREIyguaFi+eIPkIQZsnU92E9df48Jg/4uT9LbiSYG1ZuIgg6fhjlU\nTgmTo/w6MQKBgQCsq+oUxys52dbj0K+v7sNpvHMgukkAGXyrsQJrn9XznqXeWu0t\nzyazGIKj1MGuUGUY9D/KL2l+dQ9jHCvg0ujHIWN8NK/hmCjvrneBd6g9KEY/wcLN\nxodyRvyBKBtaHJ1jLu5lj2CqAyGYrOfOVpg9IuY5nuUxdXn+91FVxc1n4QKBgGdP\nuKxQWlnoXTWcHarS3PrSNca2qx9TVkX5NC9hdWHlBK7BEUC9r/ui6ADjNqEvNX1v\nOpqIqRjUnSU10NNcvxc1nhN50Ws7iJy/uGcS9p+4y10fzoRwVx2mZ5koMLOF7WYm\n1e9RbJB4Mwkw+QfVbT2S/5IswXgdAMpaZP1Pv9PBAoGAHVS7eyVg7zQU4xVNgzQH\nadYPWlZ/8WvhgNtq8ooP9SZ8+SgbOorkXORB9am7PTKP9WHLlnUn8XBLCXmcSfTu\n5fH1LPjSxBorZGLtz3R1ycUSZvNVaj4WIxwSACVxkI+I2isT8bCylhutUbP51QTE\niqErTApEYOeo/s2HJ0+0SfE=\n-----END PRIVATE KEY-----\n";
const string iss = "wonderpoker@api-4732284401405137027-508708.iam.gserviceaccount.com";
#else
const string private_key = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDQRT2kbtoUgcfn\nol+TeNXviXjK8/CWoqM+35xpjHfLV0jY0nEYj4xlz2C/D8TLoMxumwX5/TjU8nqS\nNAf6D4g8uQnW8W1+uvDUNLBzi4DZU5O9COUMPlt+pf3wrfOLSSi9dlmONi/fcKvJ\ny3/ClhvdH0pnsnSoNRX/mtFgQUkNySsOh3gxNBOnSsuiJq3UBEUwvdVQzmM5gp2m\nPM4RpjtF6ipq3c43YAQmy1t47aupeoKP4d2u+tpKh0AhgbwFc3E/pcEpM8fEM2G8\n7LM4HI2URG+G1mgs/2syFoiYbnXiSFrynO5Y/xhsVEAGUQbbTOhXOIo06r28qjI+\nSGcJljWVAgMBAAECggEAI5FjntiYkU59rYsPIAAPpyYW1TA2Sv9w+hyVmXf2AGnC\nZNcBngRr4Xo6Rz7h6LWoHA6JBYGvijxz3tKYdyXeaiJeshDOlolF9jOeW8k8AsFx\ndmPVvBLNLNKbzNXkRQoENWEIF8+cut/C0sZYZgqdLRT6L73sCsrwHBQcREA6Aeql\nY2AIpgwENGoos3hIH5U4zYtHluWQjuPkBlWMhT0m5YRC04ReXAB+5HjB7OtsVKb9\nwGjaEMqgsIedGDp3II57QoKkbmA8v/9VRehrqhTVWTA+rtc+E9dL+hoVE1C8OkTi\nQsvtIeEWh5k+q/VpZyaJHkxwK6yc0uZ4+zPWQVc2wQKBgQDyi/QKAKCDCGRQXgQ2\nmMpxui4nELFeIC/llDLUsXf7WUJG9WI72A+CVlRd22NiiL3TGOgIjzsZxc1izScV\n5eHl10DrbBFEj3Eo1UuTvryNK/d+xQELVmNdY5NFvkaSlIxNe55XvEWWkkhqWUu7\nfVhEux6LttrnT8YpZSoq1+yu1QKBgQDb0pTkzSk3jhAul89gpzIQMjtp4Qy9uY+W\nAHKtsCzIYXbnZ2N0feM0jk6ZcG08qoWtmN8MgH3WmUU8+IGdx6denHvM+9fKFA3c\n28PsKg6sMcIAZQRCg+06gi22UTg8QawZF3MmjmavsMzL2l1LrKPZ8U1T1aoiCwiN\n0O2S6DZLwQKBgEopnwIb9Ph77WhhhvpGt8gUfJ43TXSRdPPG39AHP7+cKFbYGyRf\nSUa6LOvR9E6hryFIAVL0sMaNo+JtBmF8vBjyO4iAr7r9/UTWC1juvmqQIgoiJNKZ\npKJemx59MskJIOlkPiNnjwOeUQQrr6txhTbm3E0uKTsHOnlX1I/rHBTNAoGBALlB\n/7nULahkUb7yshMC1W5Q50GUKOi8OgZw2jUHBLbbbRoYR/klHxZr2FH4LheynnZv\nH4upvmPdSDzaMmKGoMdbmfpnRLVJsWUw0S13RGZaiOVMPQrRpFxq2ZuNV5dwwVlk\nKeeUm3X1yqMCpR3hoTVL559/sqJ+Kfda3N2yVhABAoGAQqs8/MS+p8xAWQ18iMvF\nIzUAE8v5B6gBfgKB560XmV/L0y620ORecPrl1V/lMd4ZrmslkKZj30G3pyEE6IrU\nHxTEy8XVwq9Rq53nkArAUC0PQERoIfBcIz1dmhup7icK45X3wAnze7eKdYUbEEG4\nlHc2SQBzAEY5MqLARHPf048=\n-----END PRIVATE KEY-----\n";
const string iss = "demo-563@api-4965915972919737589-326317.iam.gserviceaccount.com";
#endif // __NASH_RELEASE__

#ifdef __USE_PROXY__
const string proxy_addr = "10.10.10.23:8090";
#else
const string proxy_addr = "";
#endif

namespace
{
    const Value &getValue(const Value &d, const string &key)
    {
        if (!d.HasMember(key.c_str()))
        {
            throw logic_error("property " + key + " not exsist.");
        }

        return d[key.c_str()];
    }

    template<typename T>
    T parseValue(const Value &v)
    {
        throw logic_error("must pass value type.");
    }

    template<>
    int parseValue<int>(const Value &v)
    {
        if (!v.IsInt())
        {
            throw logic_error("Not integer Type.");
        }

        return v.GetInt();
    }

    template<>
    string parseValue<string>(const Value &v)
    {
        if (!v.IsString())
        {
            throw logic_error("Not String Type.");
        }

        return v.GetString();
    }

    //产生uuid串
    string generateUUIDStr()
    {
        uuid_t uuid;
        uuid_generate(uuid);

        char buf[1024];
        memset(buf, 0, sizeof(buf));

        uuid_unparse(uuid, buf);

        string strRet;
        strRet.assign(buf, strlen(buf));

        return strRet;
    }

    std::map<std::string, std::string> parseFieldRow(vector<dbagent::TField> &fields)
    {
        map<string, string> result;
        for (auto &field : fields)
        {
            result[field.colName] = field.colValue;
        }

        return result;
    }

    int constructTFields(const std::vector<std::map<std::string, std::string>> &param, std::vector<dbagent::TField> &fields)
    {
        fields.clear();

        for (auto &m : param)
        {
            dbagent::TField field;
            field.colArithType = E_NONE;

            auto it = m.find("colName");
            if (it != m.end())
                field.colName = it->second;

            it = m.find("colType");
            if (it != m.end())
                field.colType = (dbagent::Eum_Col_Type)(S2I(it->second));

            it = m.find("colValue");
            if (it != m.end())
                field.colValue = it->second;

            it = m.find("colAs");
            if (it != m.end())
                field.colAs = it->second;

            it = m.find("colArithType");
            if (it != m.end())
                field.colArithType = (dbagent::Eum_Col_Arith_Type)(S2I(it->second));

            //保存
            fields.push_back(field);
        }

        return 0;
    }

    // map 打印为JSON
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

    // vector 打印为JSON
    template<typename Value>
    std::string VectorToString(const Value &param)
    {
        ostringstream os;
        os << param;

        return os.str();
    }

    template<>
    std::string VectorToString<std::string>(const std::string &param)
    {
        ostringstream os;
        os << "\"" << param << "\"";

        return os.str();
    }

    template<typename Value>
    std::string VectorToString(const std::vector<Value> &param)
    {
        ostringstream os;
        os << "[";
        size_t len = param.size();
        size_t idx = 0;
        for (auto &elem : param)
        {
            os << VectorToString(elem);
            if (idx != len - 1)
            {
                os << ",";
            }
            idx += 1;
        }
        os << "]";

        return os.str();
    }

    template<>
    std::string VectorToString<map<string, string>>(const map<string, string> &param)
    {
        return mToString(param);
    }

    // 构造WriteData
    template<typename T>
    void make_write_data(vector<map<string, string>> &data, const string &colName, dbagent::Eum_Col_Type colType, const T &colValue)
    {
        ostringstream os;
        os << colValue;
        map<string, string> m;
        m.insert(make_pair("colName", colName));
        m.insert(make_pair("colType", I2S(colType)));
        m.insert(make_pair("colValue", os.str()));
        data.push_back(m);
    }

    template<typename T, typename... Args>
    void make_write_data(vector<map<string, string>> &data, const string &colName, dbagent::Eum_Col_Type colType, const T &colValue, Args... args)
    {
        ostringstream os;
        os << colValue;
        map<string, string> m;
        m.insert(make_pair("colName", colName));
        m.insert(make_pair("colType", I2S(colType)));
        m.insert(make_pair("colValue", os.str()));
        data.push_back(m);
        make_write_data(data, args...);
    }

    // QUERY redis
    void readProxyData(const string &orderNum, std::map<std::string, std::string> &data)
    {
        FUNC_ENTRY("");

        dataproxy::TReadDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER) + ":" + orderNum;
        dataReq.operateType = dataproxy::E_REDIS_READ;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(orderNum);

        //填入需要读取的字段,Redis里只有hash的数据类型才有字段
        vector<map<string, string>> dataVec =
        {
            { { "colName", "order_num" },       { "colType", I2S(dbagent::STRING) } },
            { { "colName", "channel_type" },    { "colType", I2S(dbagent::INT) }        },
            { { "colName", "is_sandbox" },      { "colType", I2S(dbagent::INT) }        },
            { { "colName", "has_ack" },         { "colType", I2S(dbagent::INT) }        },
            { { "colName", "uid" },             { "colType", I2S(dbagent::BIGINT) } },
            { { "colName", "data" },            { "colType", I2S(dbagent::STRING) }  },
            { { "colName", "qrCode" },          { "colType", I2S(dbagent::INT) }        },
            { { "colName", "product_id" },      { "colType", I2S(dbagent::STRING) } },
            { { "colName", "google_order_num"}, { "colType", I2S(dbagent::STRING) } },
            { { "colName", "order_status" },    { "colType", I2S(dbagent::INT) } }
        };

        constructTFields(dataVec, dataReq.fields);
        dataproxy::TReadDataRsp dataRsp;
        int iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(orderNum)->redisRead(dataReq, dataRsp);
        ROLLLOG_DEBUG << "read redis data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read redis err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("read order err.");
        }

        if (!dataRsp.fields.empty())
        {
            vector<dbagent::TField> &row = dataRsp.fields.at(0);
            data = parseFieldRow(row);
        }

        FUNC_EXIT("", 0);
    }

    // 读取google的订单号
    void readGoogleOrderNum(const string &googleOrderNum, std::map<std::string, std::string> &data)
    {
        FUNC_ENTRY("");

        dataproxy::TReadDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER_GOOGLE) + ":" + googleOrderNum;
        dataReq.operateType = dataproxy::E_REDIS_READ;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(googleOrderNum);

        vector<map<string, string>> dataVec =
        {
            { { "colName", "google_order_num" }, { "colType", I2S(dbagent::STRING) } },
            { { "colName", "order_num" },       { "colType", I2S(dbagent::STRING) } }
        };

        constructTFields(dataVec, dataReq.fields);
        dataproxy::TReadDataRsp dataRsp;
        int iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(googleOrderNum)->redisRead(dataReq, dataRsp);
        ROLLLOG_DEBUG << "read google order data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read google order data err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("read order err");
        }

        if (!dataRsp.fields.empty())
        {
            vector<dbagent::TField> &row = dataRsp.fields.at(0);
            data = parseFieldRow(row);
        }

        FUNC_EXIT("", 0);
    }

    // 插入google订单号
    void WriteGoogleOrderNum(const string &googleOrderNum, const vector<map<string, string>> &dataVec)
    {
        FUNC_ENTRY("");

        int iRet = 0;

        string param = VectorToString(dataVec);
        ROLLLOG_DEBUG << "param:" << param << endl;
        dataproxy::TWriteDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER_GOOGLE) + ":" + googleOrderNum;
        dataReq.operateType = dataproxy::E_REDIS_INSERT;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(googleOrderNum);
        constructTFields(dataVec, dataReq.fields);
        dataproxy::TWriteDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(googleOrderNum)->redisWrite(dataReq, dataRsp);
        ROLLLOG_DEBUG << "write tb_google_order rsp, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "write tb_google_order err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("write tb_google_order err");
        }

        FUNC_EXIT("", iRet);
    }

    // 读取ios的订单号
    void readIosOrderNum(const string &iosOrderNum, std::map<std::string, std::string> &data)
    {
        FUNC_ENTRY("");

        dataproxy::TReadDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER_IOS) + ":" + iosOrderNum;
        dataReq.operateType = dataproxy::E_REDIS_READ;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(iosOrderNum);

        vector<map<string, string>> dataVec =
        {
            { { "colName", "ios_order_num" }, { "colType", I2S(dbagent::STRING) } },
            { { "colName", "order_num" },       { "colType", I2S(dbagent::STRING) } }
        };

        constructTFields(dataVec, dataReq.fields);
        dataproxy::TReadDataRsp dataRsp;
        int iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(iosOrderNum)->redisRead(dataReq, dataRsp);
        ROLLLOG_DEBUG << "read ios order data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "read ios order data err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("read order err.");
        }

        if (!dataRsp.fields.empty())
        {
            vector<dbagent::TField> &row = dataRsp.fields.at(0);
            data = parseFieldRow(row);
        }

        FUNC_EXIT("", 0);
    }

    // 插入ios订单号
    void WriteIosOrderNum(const string &iosOrderNum, const vector<map<string, string>> &dataVec)
    {
        FUNC_ENTRY("");

        int iRet = 0;

        string param = VectorToString(dataVec);
        dataproxy::TWriteDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER_IOS) + ":" + iosOrderNum;
        dataReq.operateType = dataproxy::E_REDIS_INSERT;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(iosOrderNum);
        constructTFields(dataVec, dataReq.fields);
        dataproxy::TWriteDataRsp dataRsp;
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(iosOrderNum)->redisWrite(dataReq, dataRsp);
        ROLLLOG_DEBUG << "write tb_ios_order rsp, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "write tb_ios_order err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("write tb_ios_order err");
        }

        FUNC_EXIT("", iRet);
    }

    void writeProxyData(const string &orderNum, dataproxy::Eum_Redis_Operate_Type operrateType, const vector<map<string, string>> &dataVec, dataproxy::TWriteDataRsp &dataRsp)
    {
        FUNC_ENTRY("");

        int iRet = 0;

        string param = VectorToString(dataVec);
        ROLLLOG_DEBUG << "param : " << param << endl;
        dataproxy::TWriteDataReq dataReq;
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(ORDER) + ":" + orderNum;
        dataReq.operateType = operrateType;
        dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(orderNum);
        constructTFields(dataVec, dataReq.fields);
        iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(orderNum)->redisWrite(dataReq, dataRsp);
        ROLLLOG_DEBUG << "write tb_order rsp, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
        if (iRet != 0 || dataRsp.iResult != 0)
        {
            ROLLLOG_ERROR << "write tb_order err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
            throw logic_error("write tb_order err");
        }

        FUNC_EXIT("", iRet);
    }

    // UPDATE redis
    // YOUR CODE

    // 占位符替换,类似Python
    //Usage :   auto t = formatString("INSERT INTO {0} WHERE `id` = {1}, `value` = {0}, `name` = {2}", toStringVec(111, 999, "wzc"));
    std::vector<std::string> subsVec;
    std::string formatString(std::string target, const std::vector<std::string> &subs)
    {
        for (size_t i = 0; i < subs.size(); ++i)
        {
            std::string placeholders = "{" + std::to_string(i) + "}";
            const std::string &sub = subs.at(i);
            size_t pos = target.find(placeholders);
            size_t placeholders_len = placeholders.length();
            while (pos != std::string::npos)
            {
                target.replace(pos, placeholders_len, sub);
                pos = target.find(placeholders);
            }
        }

        return target;
    }

    template<typename T>
    std::vector<std::string> toStringVec(T t)
    {
        ostringstream os;
        os << t;
        subsVec.push_back(os.str());
        std::vector<std::string> result = subsVec;
        subsVec.clear();

        return result;
    }

    template<typename T, typename... Args>
    std::vector<std::string> toStringVec(T head, Args... args)
    {
        ostringstream os;
        os << head;
        subsVec.push_back(os.str());

        return toStringVec(args...);
    }

    string getAccessToken()
    {
        FUNC_ENTRY("");

        jwt::builder builder = jwt::create();
        builder.set_algorithm("RS256");
        builder.set_type("JWT");

        builder.set_issuer(iss.c_str());
        builder.set_payload_claim("scope", jwt::claim(string("https://www.googleapis.com/auth/androidpublisher")));
        builder.set_audience("https://oauth2.googleapis.com/token");

        auto  issue_time = std::chrono::system_clock::now();
        auto  expire_time = issue_time + std::chrono::seconds(3600);
        int64_t t = std::chrono::system_clock::to_time_t(issue_time);
        (void)t;
        int64_t t2 = std::chrono::system_clock::to_time_t(expire_time);
        (void)t2;
        builder.set_expires_at(expire_time); // exp;
        builder.set_issued_at(issue_time);// The time the assertion was issued
        //const string private_key = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDQRT2kbtoUgcfn\nol+TeNXviXjK8/CWoqM+35xpjHfLV0jY0nEYj4xlz2C/D8TLoMxumwX5/TjU8nqS\nNAf6D4g8uQnW8W1+uvDUNLBzi4DZU5O9COUMPlt+pf3wrfOLSSi9dlmONi/fcKvJ\ny3/ClhvdH0pnsnSoNRX/mtFgQUkNySsOh3gxNBOnSsuiJq3UBEUwvdVQzmM5gp2m\nPM4RpjtF6ipq3c43YAQmy1t47aupeoKP4d2u+tpKh0AhgbwFc3E/pcEpM8fEM2G8\n7LM4HI2URG+G1mgs/2syFoiYbnXiSFrynO5Y/xhsVEAGUQbbTOhXOIo06r28qjI+\nSGcJljWVAgMBAAECggEAI5FjntiYkU59rYsPIAAPpyYW1TA2Sv9w+hyVmXf2AGnC\nZNcBngRr4Xo6Rz7h6LWoHA6JBYGvijxz3tKYdyXeaiJeshDOlolF9jOeW8k8AsFx\ndmPVvBLNLNKbzNXkRQoENWEIF8+cut/C0sZYZgqdLRT6L73sCsrwHBQcREA6Aeql\nY2AIpgwENGoos3hIH5U4zYtHluWQjuPkBlWMhT0m5YRC04ReXAB+5HjB7OtsVKb9\nwGjaEMqgsIedGDp3II57QoKkbmA8v/9VRehrqhTVWTA+rtc+E9dL+hoVE1C8OkTi\nQsvtIeEWh5k+q/VpZyaJHkxwK6yc0uZ4+zPWQVc2wQKBgQDyi/QKAKCDCGRQXgQ2\nmMpxui4nELFeIC/llDLUsXf7WUJG9WI72A+CVlRd22NiiL3TGOgIjzsZxc1izScV\n5eHl10DrbBFEj3Eo1UuTvryNK/d+xQELVmNdY5NFvkaSlIxNe55XvEWWkkhqWUu7\nfVhEux6LttrnT8YpZSoq1+yu1QKBgQDb0pTkzSk3jhAul89gpzIQMjtp4Qy9uY+W\nAHKtsCzIYXbnZ2N0feM0jk6ZcG08qoWtmN8MgH3WmUU8+IGdx6denHvM+9fKFA3c\n28PsKg6sMcIAZQRCg+06gi22UTg8QawZF3MmjmavsMzL2l1LrKPZ8U1T1aoiCwiN\n0O2S6DZLwQKBgEopnwIb9Ph77WhhhvpGt8gUfJ43TXSRdPPG39AHP7+cKFbYGyRf\nSUa6LOvR9E6hryFIAVL0sMaNo+JtBmF8vBjyO4iAr7r9/UTWC1juvmqQIgoiJNKZ\npKJemx59MskJIOlkPiNnjwOeUQQrr6txhTbm3E0uKTsHOnlX1I/rHBTNAoGBALlB\n/7nULahkUb7yshMC1W5Q50GUKOi8OgZw2jUHBLbbbRoYR/klHxZr2FH4LheynnZv\nH4upvmPdSDzaMmKGoMdbmfpnRLVJsWUw0S13RGZaiOVMPQrRpFxq2ZuNV5dwwVlk\nKeeUm3X1yqMCpR3hoTVL559/sqJ+Kfda3N2yVhABAoGAQqs8/MS+p8xAWQ18iMvF\nIzUAE8v5B6gBfgKB560XmV/L0y620ORecPrl1V/lMd4ZrmslkKZj30G3pyEE6IrU\nHxTEy8XVwq9Rq53nkArAUC0PQERoIfBcIz1dmhup7icK45X3wAnze7eKdYUbEEG4\nlHc2SQBzAEY5MqLARHPf048=\n-----END PRIVATE KEY-----\n";

        // auto signature = builder.sign(jwt::algorithm::rs256("", private_key));
        // std::vector<std::pair<std::string, std::string> > params =
        // {
        //     { "grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer" },
        //     { "assertion", signature }
        // };
        // string body = Request::urlEncode(params);
        // string response = Request::post("https://oauth2.googleapis.com/token", body, proxy_addr);
        // 
        // 
        // https://accounts.google.com/o/oauth2/token?grant_type=refresh_token&refresh_token=1//0eFE6oPys7JS3CgYIARAAGA4SNwF-L9IrOCpz0W1x-tjBOubQYsG1iyrh-GkWBcWZ5agYhLmgFpCZeKXDZDhTuf5nXVS9gckK374
        // &client_id=513019127205-cvmad2j46ugogf7k2brj7fd786ffl0il.apps.googleusercontent.com&client_secret=fDg2zgmrx7BbJk5mHP8v1HPb
        auto signature = builder.sign(jwt::algorithm::rs256("", private_key));
        std::vector<std::pair<std::string, std::string> > params =
        {
            { "grant_type", "refresh_token" },
            { "refresh_token", "1//0eFE6oPys7JS3CgYIARAAGA4SNwF-L9IrOCpz0W1x-tjBOubQYsG1iyrh-GkWBcWZ5agYhLmgFpCZeKXDZDhTuf5nXVS9gckK374" },
            { "client_id", "513019127205-cvmad2j46ugogf7k2brj7fd786ffl0il.apps.googleusercontent.com" },
            { "client_secret", "fDg2zgmrx7BbJk5mHP8v1HPb" }
        };
        string body = Request::urlEncode(params);
        string response = Request::post("https://accounts.google.com/o/oauth2/token", body, proxy_addr);

        Document d;
        if (d.Parse(response.c_str()).HasParseError())
        {
            throw logic_error("parse json error. raw data : " + response);
        }

        const Value &propertyStatus = getValue(d, "access_token");
        string access_token = parseValue<string>(propertyStatus);
        ROLLLOG_DEBUG << "issue_time : " << t << " expire_time : " << t2 << " , token : " << response << endl;

        FUNC_EXIT("", 0);

        return access_token;
    }

    // 订单日志相关
    std::string GetTimeFormat()
    {
        std::string sFormat("%Y-%m-%d %H:%M:%S");
        time_t t = time(NULL);
        struct tm *pTm = localtime(&t);
        if (pTm == NULL)
        {
            return "";
        }

        ///
        char sTimeString[255] = "\0";
        strftime(sTimeString, sizeof(sTimeString), sFormat.c_str(), pTm);
        return std::string(sTimeString);
    }

    /**
    * @brief 产生日志体.
    *
    * @para
    * @return result
    */
    // void gen_log_body(const string  &roomId,        //房间ID
    //                   const string  &orderSn,       //游戏订单号
    //                   const string  &sdkOrderSn,        //sdk平台订单号
    //                   const string  &payMoney,      //金额：分
    //                   int               status,         //充值状态： 0待支付、1已支付发币成功、2已支付发币失败、
    //                   int               payType,        //充值方式：例如0苹果商店，1谷歌商店、2脸书（后台配置）
    //                   const string  &goodList,      //购买物品列表,json格式 {"1001001":10,"1001002":20}
    //                   const string  &shopItemId,        //商品id，策划配置表shopitem.xlsx配置
    //                   const string  &ip,                //充值所属ip
    //                   int               source,         //充值来源 1.活动充值 2.非活动充值
    //                   string         &result            // 返回值
    //                  )
    // {
    //     result = formatString("{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|{8}|{9}", toStringVec(
    //                               roomId,
    //                               orderSn,
    //                               sdkOrderSn,
    //                               payMoney,
    //                               status,
    //                               payType,
    //                               goodList,
    //                               shopItemId,
    //                               ip,
    //                               source
    //                           ));
    // }

    // /**
    // * @brief 产生日志头部.
    // *
    // * @para
    // * @return result
    // */
    // void gen_head(const UserBaseInfoExt &stUserBaseInfo, int64_t uid, string &result)
    // {
    //     // 这几个值硬编码
    //     const string LOG_VERSION = "1.0";
    //     const int APP_ID = 1001;
    //     const int ZONE_ID = 0;
    //     const string GAME_VERSION = "1.0";
    //     const string LOG_TOPIC = "recharge_log";


    //     int32_t gameId = 1001;  // hard code
    //     string sdkUuid = "";    // 未接入sdk,此值为空,hard code

    //     int64_t triggerTime = tars::TC_TimeProvider::getInstance()->getNowMs();
    //     result = formatString("{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|{8}|{9}|{10}|{11}|{12}", toStringVec(
    //                               GetTimeFormat(),          // dateTime
    //                               LOG_VERSION,              // logVersion
    //                               APP_ID,                       // appId
    //                               LOG_TOPIC,                    // topic
    //                               triggerTime,              // triggerTime
    //                               stUserBaseInfo.platform,  // platform ===> pass
    //                               ZONE_ID,                  // zoneId
    //                               GAME_VERSION,             // gameVer
    //                               gameId,                       // gameId
    //                               stUserBaseInfo.channnelID,    // channelId  ===> pass
    //                               uid,                      // uid 请求包里有 ===> pass
    //                               sdkUuid,                  // sdkUuid
    //                               stUserBaseInfo.areaID     // areaId
    //                           ));
    // }

    // void log_recharge(const string &head, const string &body)
    // {
    //     string log = head + "|" + body;
    //     FDLOG_RECHARGE_LOG << log << endl;
    // }

    // // 充值方式
    // enum Eum_Recharge_Pay_type
    // {
    //  E_Recharge_IOS      = 0,
    //  E_Recharge_Google   = 1,
    //  E_Recharge_Facebook = 2
    // };

    // 充值方式
    enum Eum_Recharge_Pay_type
    {
        E_Recharge_Default =  0,
        E_Recharge_Google   = 1,
        E_Recharge_Mycard = 2,
        E_Recharge_IOS    = 3,
    };

    // 充值状态
    enum Eum_Recharge_Status
    {
        E_Pending   = 0,
        E_Purchased = 1
    };

    // 充值来源
    enum Eum_Recharge_Source
    {
        E_Activity  = 1,
        E_Other     = 2
    };

    const string INVALID_ROOM_ID = "NULL";
}

//////////////////////////////////////////////////////
void OrderServantImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void OrderServantImp::destroy()
{
    //destroy servant here:
    //...
}

//查询
static tars::Int32 selectUserAccount(long uid, userinfo::GetUserResp &resp)
{
    auto pDBAgentServant = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid);
    if (pDBAgentServant)
    {
        dataproxy::TReadDataReq dataReq;
        dataReq.resetDefautlt();
        dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(USER_ACCOUNT) + ":" + L2S(uid);
        dataReq.operateType = E_REDIS_READ;
        dataReq.clusterInfo.resetDefautlt();
        dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
        dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_STRING;
        dataReq.clusterInfo.frageFactor = tars::hash<string>()(L2S(uid));

        vector<TField> fields;
        TField tfield;
        tfield.colArithType = E_NONE;
        tfield.colName = "uid";
        tfield.colType = BIGINT;
        fields.push_back(tfield);
        tfield.colName = "username";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "password";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "safes_password";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "reg_type";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "reg_time";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "reg_ip";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "reg_device_no";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "is_robot";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "agcid";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "disabled";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "device_id";
        tfield.colType = dbagent::STRING;
        fields.push_back(tfield);
        tfield.colName = "device_type";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "platform";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "channel_id";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "area_id";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "is_forbidden";
        tfield.colType = INT;
        fields.push_back(tfield);
        tfield.colName = "forbidden_time";
        tfield.colType = STRING;
        fields.push_back(tfield);
        tfield.colName = "bindChannelId";
        tfield.colType = dbagent::INT;
        fields.push_back(tfield);
        tfield.colName = "bindOpenId";
        tfield.colType = dbagent::STRING;
        fields.push_back(tfield);
        tfield.colName = "isinwhitelist";
        tfield.colType = dbagent::INT;
        fields.push_back(tfield);
        tfield.colName = "whitelisttime";
        tfield.colType = dbagent::STRING;
        fields.push_back(tfield);
        dataReq.fields = fields;

        dataproxy::TReadDataRsp dataRsp;
        int iRet = pDBAgentServant->redisRead(dataReq, dataRsp);
        if ((iRet != 0) || (dataRsp.iResult != 0))
        {
            ROLLLOG_ERROR << "redisRead failed, iRet: " << iRet << ", dataRsp.iResult: " << dataRsp.iResult << endl;
            return -2;
        }

        if (dataRsp.fields.empty())
        {
            ROLLLOG_ERROR << "uid:" << uid << " not exist in tb_useraccount!" << endl;
            return -3;
        }

        for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
        {
            for (auto itfields = it->begin(); itfields != it->end(); ++itfields)
            {
                //ROLLLOG_DEBUG << "read user account, colName: " << itfields->colName << ", colValue: " << itfields->colValue << endl;

                if (itfields->colName == "username")
                {
                    resp.userName = itfields->colValue;
                }
                else if (itfields->colName == "device_id")
                {
                    resp.deviceID = itfields->colValue;
                }
                else if (itfields->colName == "device_type")
                {
                    resp.deviceType = itfields->colValue;
                }
                else if (itfields->colName == "platform")
                {
                    resp.platform = (userinfo::E_Platform_Type)S2I(itfields->colValue);
                }
                else if (itfields->colName == "channel_id")
                {
                    resp.channnelID = (userinfo::E_Channel_ID)S2I(itfields->colValue);
                }
                else if (itfields->colName == "area_id")
                {
                    resp.areaID = S2I(itfields->colValue);
                }
                else if (itfields->colName == "is_robot")
                {
                    resp.isRobot = S2I(itfields->colValue);
                }
                else if (itfields->colName == "reg_time")
                {
                    resp.regTime = g_app.getOuterFactoryPtr()->GetTimeTick(itfields->colValue);
                }
                else if (itfields->colName == "bindChannelId")
                {
                    resp.bindChannelId = S2I(itfields->colValue);
                }
                else if (itfields->colName == "bindOpenId")
                {
                    resp.bindOpenId = itfields->colValue;
                }
                else if (itfields->colName == "isinwhitelist")
                {
                    resp.isinwhitelist = S2I(itfields->colValue);
                }
                else if (itfields->colName == "whitelisttime")
                {
                    resp.whitelisttime = g_app.getOuterFactoryPtr()->GetTimeTick(itfields->colValue);
                }
            }
        }

        ROLLLOG_DEBUG << "get useraccount data succ: uid= " << uid << ", fields size: " << dataRsp.fields.size() << endl;
        return 0;
    }
    else
    {
        ROLLLOG_ERROR << "pDBAgentServant is null" << endl;
        return -1;
    }
}

namespace
{
    // 这个代码未实现 ==TODO
    string make_googlePlay_url(const string &strTemplate, const string &packageName, const string &productId, const string &token)
    {
        // "https://www.googleapis.com/androidpublisher/v3/applications/packageName/purchases/products/productId/tokens/token";
        return "http://10.10.10.188:8080/reference/googlePlay.html";
    }

    // IOS 的URL
    // DEBUG:   https://sandbox.itunes.apple.com/verifyReceipt
    // RELEASE: https://buy.itunes.apple.com/verifyReceipt
}

namespace
{
    // 可能会抛出异常
    int IOSVerify(const string &response, const string &cur_transaction_id, const string &cur_product_id)
    {
        Document d;
        if (d.Parse(response.c_str()).HasParseError())
        {
            throw logic_error("parse json error. raw data : " + response);
        }

        //
        const Value &propertyStatus = getValue(d, "status");
        int status = parseValue<int>(propertyStatus);
        if (status != 0)
        {
            ROLLLOG_ERROR << "verify IOS failed, reason: status failed, status: " << status << ", transaction_id : " << cur_transaction_id << endl;
            return -1; //状态码出错
        }

        // 获取嵌套的对象的属性
        const Value &property_receipt = getValue(d, "receipt");
        // IOS7+
        if (property_receipt.HasMember("in_app"))
        {
            const Value &property_in_app = getValue(property_receipt, "in_app");
            for (SizeType i = 0; i < property_in_app.Size(); i++)
            {
                const Value &item = property_in_app[i];

                const Value &property_transaction_id = getValue(item, "transaction_id");
                string transaction_id = parseValue<string>(property_transaction_id);

                const Value &property_product_id = getValue(item, "product_id");
                string product_id = parseValue<string>(property_product_id);

                // if (transaction_id == cur_transaction_id)
                if ((transaction_id == cur_transaction_id) && (product_id == cur_product_id))
                {
                    ROLLLOG_DEBUG << "verify IOS success, transaction_id: " << cur_transaction_id << endl;

                    return 0; // 校验成功
                }
                else
                {
                    ROLLLOG_ERROR << "verify err, transaction_id: " << transaction_id << ", cur_transaction_id: " << cur_transaction_id
                                  << ", product_id: " << product_id << ", cur_product_id: " << cur_product_id << endl;
                }
            }
        }
        else
        {
            const Value &property_transaction_id = getValue(property_receipt, "transaction_id");
            string transaction_id = parseValue<string>(property_transaction_id);

            const Value &property_product_id = getValue(property_receipt, "product_id");
            string product_id = parseValue<string>(property_product_id);
            // if (transaction_id == cur_transaction_id)
            if ((transaction_id == cur_transaction_id) && (product_id == cur_product_id))
            {
                ROLLLOG_DEBUG << "verify IOS success, transaction_id: " << cur_transaction_id << endl;

                return 0; // 校验成功
            }
            else
            {
                ROLLLOG_ERROR << "verify err, transaction_id: " << transaction_id << ", cur_transaction_id: " << cur_transaction_id
                              << ", product_id: " << product_id << ", cur_product_id: " << cur_product_id << endl;
            }
        }

        return 0;
    }

    order::Eum_Purchase_Type getPurchaseType(const string &product_id)
    {
        static multimap<order::Eum_Purchase_Type, string> m =
        {
            { order::E_PURCHASE_GOLD, "com.currency.wonderpoker.chip" },
            { order::E_PURCHASE_INTEGRAL, "com.currency.wonderpoker.integral" },
            { order::E_PURCHASE_LUCKPACK, "com.currency.wonderpoker.luckypack" },
            { order::E_PURCHASE_SPECIALPACK, "com.currency.wonderpoker.specialpackage" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.dailypromotion" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.limitpromotion" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.specialpromotion" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.magicemojipack1" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.magicemojipack2" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.magicemojipack3" },
            { order::E_PURCHASE_RECHARGE, "com.currency.wonderpoker.magicemojipack4" },
        };

        using MapElemType = map<order::Eum_Purchase_Type, string>::value_type;
        order::Eum_Purchase_Type type = order::E_PURCHASE_NONE;
        for_each(m.begin(), m.end(), [&type, &product_id](const MapElemType & param)->void
        {
            const string &val = param.second;
            if (product_id.find(val) != string::npos)
            {
                type = param.first;
            }
        });

        return type;
    }
}

static string getGoogleToken(const string &credential)
{
    Document d;
    if (d.Parse(credential.c_str()).HasParseError())
    {
        throw logic_error("getGoogleToken parse credential json error. raw data : " + credential);
    }

    const Value &tokenValue = getValue(d, "token");
    string token = parseValue<string>(tokenValue);
    return token;
}

// 需要保存凭据 -- 客户端可能会丢单 TODO
//校验订单
tars::Int32 OrderServantImp::orderVerify(const order::OrderVerifyReq &req, order::OrderVerifyResp &rsp, tars::TarsCurrentPtr current)
{
    //TODO 需要存放凭据,如果校验失败,下次可以继续校验
    FUNC_ENTRY("");

    int iRet = 0;

    ///
    ROLLLOG_DEBUG << ">>>orderVerify req : " << printTars(req) << endl;

    try
    {
        const string &orderNum = req.orderNum;
        std::map<std::string, std::string> data;
        readProxyData(orderNum, data);
        if (data.empty())
        {
            rsp.resultCode = -1; // 订单号不存在
        }

        //
        bool has_ack = data["has_ack"] == "1";
        if (has_ack)
        {
            rsp.resultCode = 0; // 已经确认过
            order::Eum_Channel_Type channelType = (order::Eum_Channel_Type)S2I(data.at("channel_type"));
            if (channelType == order::E_CHANNEL_IOS)
            {
                rsp.identity = req.transaction_id;
            }
            else
            {
                string token = getGoogleToken(data.at("data"));
                rsp.identity = token;
                rsp.type = channelType;
            }

            FUNC_EXIT("", iRet);

            return iRet;
        }

        userinfo::GetUserResp getUserResp;
        iRet = selectUserAccount(req.uid, getUserResp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "selectUserInfo() fail, uid:" << req.uid << endl;
            return iRet;
        }

        bool is_sandBox = S2I(data.at("is_sandbox"));

        int qrCode = S2I(data.at("qrCode"));

        string product_id = data.at("product_id");
        int goodsType = getPurchaseType(product_id);
        order::Eun_Order_Env env = is_sandBox ? order::E_ORDER_ENV_DEBUG : order::E_ORDER_ENV_RELEASE;

        // 确认
        order::Eum_Channel_Type channelType = (order::Eum_Channel_Type)S2I(data.at("channel_type"));
        std::string url = g_app.getOuterFactoryPtr()->getVerifyUrl(channelType, env);
        if (channelType == order::E_CHANNEL_IOS)
        {
            {
                //先查询,是否已经校验过
                const string &ios_order_num = req.transaction_id;
                std::map<std::string, std::string> data;
                readIosOrderNum(ios_order_num, data);

                //
                if (!data.empty())
                {
                    FUNC_EXIT("", iRet);

                    return iRet;
                }
            }

            std::string postData = req.credential;
            string response = Request::post(url, postData);
            string cur_transaction_id = req.transaction_id;
            iRet = IOSVerify(response, cur_transaction_id, product_id);
            if (iRet == 0)
            {
                {
                    //校验成功，记下订单
                    vector<map<string, string>> iosDataVec;
                    make_write_data(iosDataVec,
                                    "ios_order_num", dbagent::STRING, req.transaction_id,
                                    "order_num", dbagent::STRING, orderNum
                                   );
                    WriteIosOrderNum(req.credential, iosDataVec);
                }

                // 1. 保存交易id
                // 2. 保存json
                // 3. 设置ack为1
                {
                    //产生充值记录
                    // string head;
                    int64_t uid = req.uid;
                    const UserBaseInfoExt &stUserBaseInfo = _userInfo.at(uid);
                    // gen_head(stUserBaseInfo, uid, head);

                    //数据体
                    // string body;
                    // string roomId       = INVALID_ROOM_ID;
                    string orderSn      = req.orderNum;         //游戏订单号
                    string sdkOrderSn   = req.transaction_id;   //sdk平台订单号
                    string payMoney     = "0";
                    try
                    {
                        payMoney = g_app.getOuterFactoryPtr()->getProductPrice(qrCode); //金额：分
                    }
                    catch(const std::exception &e)
                    {
                        ROLLLOG_ERROR << "IOSPAY-->commodity do not exsist! qrCode : " << qrCode << endl;
                        // 商品不存在
                        // rsp.resultCode = -1;
                        // rsp.identity = cur_transaction_id; // IOS 返回订单号
                        // return -2;
                    }
                    ROLLLOG_DEBUG << "payMoney: " << payMoney << endl;
                    int     status      = E_Purchased;          //充值状态： 0待支付、1已支付发币成功、2已支付发币失败、
                    int     payType     = E_Recharge_IOS;       //充值方式：例如0苹果商店，1谷歌商店、2脸书（后台配置）
                    string  goodList    = I2S(qrCode);      //购买物品列表,json格式 {"1001001":10,"1001002":20}
                    string  shopItemId  = product_id;       //商品id，策划配置表shopitem.xlsx配置
                    string  ip          = _userIp[uid];             //充值所属ip
                    int     source      = E_Other;          //充值来源 1.活动充值 2.非活动充值
                    // gen_log_body(roomId,            //房间ID
                    //              orderSn,       //游戏订单号
                    //              sdkOrderSn,        //sdk平台订单号
                    //              payMoney,      //金额：分
                    //              status,            //充值状态： 0待支付、1已支付发币成功、2已支付发币失败、
                    //              payType,       //充值方式：例如0苹果商店，1谷歌商店、2脸书（后台配置）
                    //              goodList,      //购买物品列表,json格式 {"1001001":10,"1001002":20}
                    //              shopItemId,        //商品id，策划配置表shopitem.xlsx配置
                    //              ip,                //充值所属ip
                    //              source,            //充值来源 1.活动充值 2.非活动充值
                    //              body
                    //             );
                    // log_recharge(head, body);


                    //更新到DB中
                    DaqiGame::TLog2DBReq tLog2DBReq;
                    tLog2DBReq.sLogType = 25;
                    vector<string> veclog;
                    veclog.push_back(I2S(APP_ID));
                    veclog.push_back("1001");
                    veclog.push_back(I2S(stUserBaseInfo.channnelID));
                    veclog.push_back(I2S(stUserBaseInfo.areaID));
                    veclog.push_back(I2S(stUserBaseInfo.platform));
                    veclog.push_back(L2S(uid));
                    veclog.push_back(orderSn);
                    veclog.push_back(sdkOrderSn);
                    veclog.push_back(payMoney);
                    veclog.push_back(I2S(status));
                    veclog.push_back(I2S(payType));
                    veclog.push_back(goodList);
                    veclog.push_back(shopItemId);
                    veclog.push_back(ip);
                    veclog.push_back(I2S(source));
                    veclog.push_back(I2S(getUserResp.isinwhitelist));
                    tLog2DBReq.vecLogData.push_back(veclog);
                    g_app.getOuterFactoryPtr()->asyncLog2DB(uid, tLog2DBReq);
                }

                vector<map<string, string>> dataVec;
                int has_ack = 1;
                make_write_data(dataVec,
                                "has_ack",          dbagent::INT, has_ack,
                                "data",             dbagent::STRING, response,
                                "transaction_id",   dbagent::STRING, cur_transaction_id);

                ///
                dataproxy::TWriteDataRsp dataRsp;
                writeProxyData(orderNum, dataproxy::E_REDIS_WRITE, dataVec, dataRsp);

                // 发放商品
                string json_data;
                if (goodsType == order::E_PURCHASE_LUCKPACK || goodsType == order::E_PURCHASE_SPECIALPACK ||
                        goodsType == order::E_PURCHASE_GOLD  || goodsType == order::E_PURCHASE_INTEGRAL || goodsType == order::E_PURCHASE_RECHARGE)
                {
                    mall::DispatchGoodsReq dispatchGoodsReq;
                    dispatchGoodsReq.uid = req.uid;
                    dispatchGoodsReq.QRCode = qrCode;
                    mall::DispatchGoodsResp dispatchGoodsResp;
                    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid)->dispatchGoods(dispatchGoodsReq, dispatchGoodsResp);
                    if (iRet != 0)
                    {
                        THROW_LOGIC_ERROR("RPC dispatchGoods failed.");
                    }

                    bool first_step = true;
                    for (auto item : dispatchGoodsResp.data)
                    {
                        if (first_step)
                        {
                            json_data += formatString("[{\"GoodsID\":{0},\"GoodsCount\":{1}}", toStringVec(item.GoodsID, item.GoodsCount));
                            first_step = false;
                        }
                        else
                        {
                            json_data += formatString(", {\"GoodsID\":{0},\"GoodsCount\":{1}}", toStringVec(item.GoodsID, item.GoodsCount));
                        }
                    }
                    json_data += "]";
                }
                else if (goodsType == order::E_PURCHASE_PIG)
                {
                    goldpig::PurchaseGoldPigReq purchaseGoldPigReq;
                    purchaseGoldPigReq.uid = req.uid;
                    goldpig::PurchaseGoldPigResp purchaseGoldPigResp;
                    iRet = g_app.getOuterFactoryPtr()->getActivityServantPrx(req.uid)->purchase(purchaseGoldPigReq, purchaseGoldPigResp);
                    if (iRet != 0 || purchaseGoldPigResp.resultCode != 0)
                    {
                        THROW_LOGIC_ERROR("RPC goldPig purchase failed.");
                    }
                    json_data = to_string(purchaseGoldPigResp.storage);
                }
                else
                {
                    THROW_LOGIC_ERROR("invalid GoodsType : ", goodsType);
                }

                //
                rsp.resultCode = 0;
                rsp.identity = cur_transaction_id; // IOS 返回订单号
                rsp.json = formatString("{\"type\":{0},\"data\":{1}}", toStringVec(goodsType == order::E_PURCHASE_PIG ? order::E_PURCHASE_PIG : order::E_PURCHASE_GOLD, json_data));
            }
        }
        else if (channelType == order::E_CHANNEL_GOOGLEPLAY)
        {
            Document d;
            string credential = req.credential;
            if (d.Parse(credential.c_str()).HasParseError())
            {
                throw logic_error("parse credential json error. raw data : " + credential);
            }

            //将这些数据保存起来--补单的时候需要
            const Value &packageNameValue = getValue(d, "packageName");
            string packageName = parseValue<string>(packageNameValue);
            const Value &productIdValue = getValue(d, "productId");
            string productId = parseValue<string>(productIdValue);
            const Value &tokenValue = getValue(d, "token");
            string token = parseValue<string>(tokenValue);

            ///
            string access_token = getAccessToken();
            url = formatString(url, toStringVec(packageName, productId, token, access_token));
            ROLLLOG_DEBUG << "url : " << url << endl;
            string response = Request::get(url, proxy_addr);

            // 校验购买状态
            Document getRspDoc;
            if (getRspDoc.Parse(response.c_str()).HasParseError())
            {
                throw logic_error("parse response json error. raw data : " + response);
            }

            //
            const Value &purchaseStateValue = getValue(getRspDoc, "purchaseState");
            int purchaseState = parseValue<int>(purchaseStateValue);
            if (purchaseState != 0)
            {
                iRet = -2;
                rsp.identity = token;
                ROLLLOG_ERROR << "have not purchase!" << endl;
                FUNC_EXIT("", iRet);
                return iRet;
            }

            // 记录充值
            {
                //产生充值记录
                // string head;
                int64_t uid = req.uid;
                const UserBaseInfoExt &stUserBaseInfo = _userInfo.at(uid);
                // gen_head(stUserBaseInfo, uid, head);

                //数据体
                // string body;
                // string roomId = INVALID_ROOM_ID;
                string orderSn = req.orderNum;      //游戏订单号
                string sdkOrderSn = data.at("google_order_num");        //sdk平台订单号
                string payMoney = "0";
                try
                {
                    payMoney = g_app.getOuterFactoryPtr()->getProductPrice(qrCode);     //金额：分
                }
                catch(const std::exception &e)
                {
                    ROLLLOG_ERROR << "GOOGLEPLAY-->commodity do not exsist! qrCode : " << qrCode << endl;
                }

                ROLLLOG_DEBUG << "payMoney: " << payMoney << endl;

                int     status = E_Purchased;           //充值状态： 0待支付、1已支付发币成功、2已支付发币失败、
                int     payType = E_Recharge_Google;    //充值方式：例如0苹果商店，1谷歌商店、2脸书（后台配置）
                string  goodList = I2S(qrCode);     //购买物品列表,json格式 {"1001001":10,"1001002":20}
                string  shopItemId = product_id;    //商品id，策划配置表shopitem.xlsx配置
                string  ip = _userIp[uid];          //充值所属ip
                int     source = E_Other;           //充值来源 1.活动充值 2.非活动充值
                // gen_log_body(roomId,                //房间ID
                //              orderSn,       //游戏订单号
                //              sdkOrderSn,        //sdk平台订单号
                //              payMoney,      //金额：分
                //              status,            //充值状态： 0待支付、1已支付发币成功、2已支付发币失败、
                //              payType,       //充值方式：例如0苹果商店，1谷歌商店、2脸书（后台配置）
                //              goodList,      //购买物品列表,json格式 {"1001001":10,"1001002":20}
                //              shopItemId,        //商品id，策划配置表shopitem.xlsx配置
                //              ip,                //充值所属ip
                //              source,            //充值来源 1.活动充值 2.非活动充值
                //              body
                //             );
                // log_recharge(head, body);

                //更新到DB中
                DaqiGame::TLog2DBReq tLog2DBReq;
                tLog2DBReq.sLogType = 25;
                vector<string> veclog;
                veclog.push_back(I2S(APP_ID));
                veclog.push_back("1001");
                veclog.push_back(I2S(stUserBaseInfo.channnelID));
                veclog.push_back(I2S(stUserBaseInfo.areaID));
                veclog.push_back(I2S(stUserBaseInfo.platform));
                veclog.push_back(L2S(uid));
                veclog.push_back(orderSn);
                veclog.push_back(sdkOrderSn);
                veclog.push_back(payMoney);
                veclog.push_back(I2S(status));
                veclog.push_back(I2S(payType));
                veclog.push_back(goodList);
                veclog.push_back(shopItemId);
                veclog.push_back(ip);
                veclog.push_back(I2S(source));
                veclog.push_back(I2S(getUserResp.isinwhitelist));
                tLog2DBReq.vecLogData.push_back(veclog);
                g_app.getOuterFactoryPtr()->asyncLog2DB(uid, tLog2DBReq);
            }

            // 保存JSON
            StringBuffer s;
            Writer<StringBuffer> writer(s);

            writer.StartObject();
            writer.String("packageName");
            writer.String(packageName.c_str());
            writer.String("productId");
            writer.String(productId.c_str());
            writer.String("token");
            writer.String(token.c_str());
            writer.EndObject();
            string getReqParams = s.GetString();
            // 1. 保存交易id
            // 2. 保存json
            // 3. 设置ack为1
            vector<map<string, string>> dataVec;
            string cur_transaction_id = req.transaction_id;
            int has_ack = 1;
            int order_status = 2;// TODO 到时候用枚举表示 已支付,待发货
            make_write_data(dataVec,
                            "has_ack", dbagent::INT, has_ack,
                            "data", dbagent::STRING, getReqParams,
                            "transaction_id", dbagent::STRING, cur_transaction_id,
                            "order_status", dbagent::INT, order_status);

            dataproxy::TWriteDataRsp dataRsp;
            ROLLLOG_DEBUG << "writeProxyData orderNum : " << orderNum << endl;
            writeProxyData(orderNum, dataproxy::E_REDIS_WRITE, dataVec, dataRsp);
            // 发放商品
            rsp.resultCode = 0;
            rsp.identity = token;
        }
        else
        {
            throw logic_error("invalid channelType.");
        }

        rsp.type = channelType;
    }
    catch (const std::exception &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        iRet = -1;
        rsp.identity = req.transaction_id;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

//出现异常了服务不会崩溃,但是也不会响应客户端
//产生订单
tars::Int32 OrderServantImp::orderYield(const order::OrderYieldReq &req, order::OrderYieldResp &rsp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        ///
        ROLLLOG_DEBUG << "type : " << req.type << ", isSandBox : " << req.isSandBox << ", qrCode: " << req.qrCode << ", uid : " << req.uid << endl;

        // GOOGLE 的补单流程有点奇怪
        if (req.type == order::E_CHANNEL_GOOGLEPLAY)
        {
            // 先查询
            const string &google_order_num = req.google_order_num;
            std::map<std::string, std::string> data;
            readGoogleOrderNum(google_order_num, data);

            // 补单
            if (!data.empty())
            {
                string order_num = data.at("order_num");
                rsp.orderNum = order_num;
                rsp.resultCode = 0;
                FUNC_EXIT("", iRet);
                return iRet;
            }
        }

        string orderNum = generateUUIDStr();
        int has_ack = 0;
        vector<map<string, string>> dataVec;
        make_write_data(dataVec,
                        "order_num", dbagent::STRING, orderNum,
                        "channel_type", dbagent::INT, req.type,
                        "is_sandbox", dbagent::INT, req.isSandBox,
                        "has_ack", dbagent::INT, has_ack,
                        "uid", dbagent::BIGINT, req.uid,
                        "qrCode", dbagent::INT, req.qrCode,
                        "product_id", dbagent::STRING, req.product_id,
                        "google_order_num", dbagent::STRING, req.google_order_num);

        dataproxy::TWriteDataRsp dataRsp;
        ROLLLOG_DEBUG << "writeProxyData orderNum : " << orderNum << endl;
        writeProxyData(orderNum, dataproxy::E_REDIS_INSERT, dataVec, dataRsp);

        if (req.type == order::E_CHANNEL_GOOGLEPLAY)
        {
            vector<map<string, string>> googleDataVec;
            make_write_data(googleDataVec, "google_order_num", dbagent::STRING, req.google_order_num, "order_num", dbagent::STRING, orderNum);
            WriteGoogleOrderNum(req.google_order_num, googleDataVec);
        }

        rsp.orderNum = orderNum;
        rsp.resultCode = 0;
    }
    catch (const std::exception &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        iRet = -1;
    }

    FUNC_EXIT("", iRet);

    return iRet;
}

//tcp请求处理接口
tars::Int32 OrderServantImp::onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current)
{
    int iRet = 0;

    try
    {
        ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", addr : " << stClientParam.sAddr << endl;

        OrderServantImp::async_response_onRequest(current, 0);

        XGameComm::TPackage pkg;
        pbToObj(sMsgPack, pkg);
        if (pkg.vecmsghead_size() == 0)
        {
            ROLLLOG_DEBUG << "package empty." << endl;
            return -1;
        }

        ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", msg : " << logPb(pkg) << endl;

        _userInfo[lUin] = stUserBaseInfo;
        _userIp[lUin] = stClientParam.sAddr;
        for (int i = 0; i < pkg.vecmsghead_size(); ++i)
        {
            int64_t ms1 = TNOWMS;

            auto &head = pkg.vecmsghead(i);
            switch (head.nmsgid())
            {
            case XGameProto::ActionName::ORDER_YIELD:  //// 生成订单号
            {
                orderProto::OrderYieldReq orderYieldReq;
                pbToObj(pkg.vecmsgdata(i), orderYieldReq);
                iRet = onOrderYield(pkg, sCurServrantAddr, orderYieldReq, current);
                break;
            }
            case XGameProto::ActionName::ORDER_VERRITY:  //// 校验订单号
            {
                orderProto::OrderVerifyReq orderVerifyReq;
                pbToObj(pkg.vecmsgdata(i), orderVerifyReq);
                iRet = onOrderVerify(pkg, sCurServrantAddr, orderVerifyReq, current);
                break;
            }
            case XGameProto::ActionName::ORDER_CONSUME: //// 订单消费
            {
                orderProto::ConsumptionVerifyReq consumptionVerifyReq;
                pbToObj(pkg.vecmsgdata(i), consumptionVerifyReq);
                iRet = onConsumptionVerify(pkg, sCurServrantAddr, consumptionVerifyReq, current);
                break;
            }
            default:
            {
                ROLLLOG_ERROR << "invalid msg id, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
                break;
            }
            }

            if (iRet != 0)
            {
                ROLLLOG_ERROR << "msg process fail, uid: " << lUin << ", msg id: " << head.nmsgid() << ", iRet: " << iRet << endl;
            }
            else
            {
                ROLLLOG_DEBUG << "msg process fail, uid: " << lUin << ", msg id: " << head.nmsgid() << ", iRet: " << iRet << endl;
            }

            int64_t ms2 = TNOWMS;
            if ((ms2 - ms1) > COST_MS)
            {
                ROLLLOG_WARN << "@Performance, msgid:" << head.nmsgid() << ", costTime:" << (ms2 - ms1) << endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        iRet = -1;
    }

    return iRet;
}

// TODO 在这里要打印用户的请求参数并且检查参数是否合法
//校验订单
int OrderServantImp::onOrderVerify(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const orderProto::OrderVerifyReq &req, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;
    orderProto::OrderVerifyResp resp;

    try
    {
        order::OrderVerifyReq orderVerifyReq;
        orderVerifyReq.orderNum = req.ordernum();
        orderVerifyReq.credential = req.credential();
        orderVerifyReq.uid = pkg.stuid().luid();
        orderVerifyReq.transaction_id = req.transaction_id();

        order::OrderVerifyResp orderVerifyResp;
        iRet = orderVerify(orderVerifyReq, orderVerifyResp, current);
        if (iRet == -2) // GOOGLE 购买失败返回Token
        {
            resp.set_identity(orderVerifyResp.identity);
        }

        if (iRet != 0)
        {
            resp.set_identity(orderVerifyResp.identity);
            ROLLLOG_DEBUG << "order verify err, req: " << logPb(req) << ", resp: " << logPb(resp) << endl;
            throw logic_error("invoke orderVerify failed.");
        }

        resp.set_resultcode(0);
        resp.set_identity(orderVerifyResp.identity);
        resp.set_json(orderVerifyResp.json);
        resp.set_type(static_cast<orderProto::Eum_Channel_Type>(orderVerifyResp.type));
    }
    catch (const std::exception &e)
    {
        resp.set_resultcode(-1);
        iRet = -1;
        ROLLLOG_ERROR << e.what() << endl;
    }

    //int rscode = resp.resultcode();
    //string identity = resp.identity();
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ORDER_VERRITY, XGameComm::MSGTYPE_RESPONSE, resp);

    FUNC_EXIT("", iRet);

    return iRet;
}

//产生订单
int OrderServantImp::onOrderYield(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const orderProto::OrderYieldReq &req, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    ROLLLOG_DEBUG << "uid : " << pkg.stuid().luid() << ", req: " << logPb(req) << endl;

    orderProto::OrderYieldResp resp;

    try
    {
        order::OrderYieldReq orderYieldReq;
        orderYieldReq.type = (order::Eum_Channel_Type)req.type();
        orderYieldReq.isSandBox = req.issandbox();
        orderYieldReq.uid = pkg.stuid().luid();
        orderYieldReq.qrCode = req.qrcode();
        orderYieldReq.product_id = req.product_id();
        orderYieldReq.google_order_num = req.google_order_num();

        order::OrderYieldResp orderYieldResp;
        iRet = orderYield(orderYieldReq, orderYieldResp, current);
        if (iRet != 0)
            throw logic_error("invoke orderYield faild.");

        //应答
        resp.set_ordernum(orderYieldResp.orderNum);
        resp.set_type(req.type());
        resp.set_product_id(req.product_id());
        resp.set_resultcode(0);
    }
    catch (const std::exception &e)
    {
        resp.set_resultcode(-1);
        iRet = -1;
        ROLLLOG_ERROR << e.what() << endl;
    }

    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ORDER_YIELD, XGameComm::MSGTYPE_RESPONSE, resp);

    FUNC_EXIT("", iRet);
    return iRet;
}

//消费，补单
int OrderServantImp::onConsumptionVerify(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const orderProto::ConsumptionVerifyReq &req, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;

    orderProto::ConsumptionVerifyResp resp;

    try
    {
        order::ConsumptionVerifyReq consumptionVerifyReq;
        consumptionVerifyReq.orderNum = req.ordernum();
        consumptionVerifyReq.uid = pkg.stuid().luid();
        order::ConsumptionVerifyResp consumptionVerifyResp;
        iRet = consumptionVerify(consumptionVerifyReq, consumptionVerifyResp, current);
        if (iRet != 0)
        {
            throw logic_error("invoke ConsumptionVerify failed.");
        }

        resp.set_resultcode(0);
        resp.set_json(consumptionVerifyResp.json);
        resp.set_ordernum(req.ordernum());
    }
    catch (const std::exception &e)
    {
        resp.set_resultcode(-1);
        iRet = -1;
        ROLLLOG_ERROR << e.what() << endl;
    }

    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ORDER_CONSUME, XGameComm::MSGTYPE_RESPONSE, resp);

    FUNC_EXIT("", iRet);
    return iRet;
}

//发送消息到客户端
template<typename T>
int OrderServantImp::toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, XGameComm::MSGTYPE type, const T &t)
{
    XGameComm::TPackage rsp;
    auto mh = rsp.add_vecmsghead();
    mh->set_nmsgid(actionName);
    mh->set_nmsgtype(type); //此处根据实际情况更改
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_ORDER); //此处根据实际情况更改
    rsp.add_vecmsgdata(pbToString(t));

    auto pPushPrx = Application::getCommunicator()->stringToProxy<JFGame::PushPrx>(sCurServrantAddr);
    if (pPushPrx)
    {
        ROLLLOG_DEBUG << "response : " << t.DebugString() << endl;
        pPushPrx->tars_hash(tPackage.stuid().luid())->async_doPushBuf(NULL, tPackage.stuid().luid(), pbToString(rsp));
    }
    else
    {
        ROLLLOG_ERROR << "pPushPrx is null : " << t.DebugString() << endl;
    }

    return 0;
}

//消费，补单
tars::Int32 OrderServantImp::consumptionVerify(const order::ConsumptionVerifyReq &req, order::ConsumptionVerifyResp &rsp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        //查询订单号,如果存在直接发货
        const string &orderNum = req.orderNum;
        std::map<std::string, std::string> data;
        readProxyData(orderNum, data);
        if (data.empty())
        {
            rsp.resultCode = -1; // 订单号不存在
        }

        bool has_ack = S2I(data.at("has_ack"));
        (void)has_ack;
        int order_status = S2I(data.at("order_status"));
        if (order_status != 2) // TODO 必须用枚举类型表示
        {
            iRet = -2; //
            FUNC_EXIT("", iRet);
            return iRet;
        }

        string orderData = data.at("data");
        // 校验是否已消费
        Document d;
        if (d.Parse(orderData.c_str()).HasParseError())
            throw logic_error("parse json error. raw data : " + orderData);

        const Value &packageNameValue = getValue(d, "packageName");
        string packageName = parseValue<string>(packageNameValue);
        const Value &productIdValue = getValue(d, "productId");
        string productId = parseValue<string>(productIdValue);
        const Value &tokenValue = getValue(d, "token");
        string token = parseValue<string>(tokenValue);

        string access_token = getAccessToken();
        std::string url = g_app.getOuterFactoryPtr()->getVerifyUrl(order::E_CHANNEL_GOOGLEPLAY, order::E_ORDER_ENV_RELEASE);
        url = formatString(url, toStringVec(packageName, productId, token, access_token));
        ROLLLOG_DEBUG << "url : " << url << endl;
        string response = Request::get(url, proxy_addr);

        // 校验消费状态
        Document getRspDoc;
        if (getRspDoc.Parse(response.c_str()).HasParseError())
        {
            throw logic_error("parse json error. raw data : " + response);
        }

        const Value &consumptionStateValue = getValue(getRspDoc, "consumptionState");
        int consumptionState = parseValue<int>(consumptionStateValue);
        if (consumptionState != 1)
        {
            iRet = -2;
            ROLLLOG_ERROR << "have not consume!" << endl;
            FUNC_EXIT("", iRet);
            return iRet;
        }

        // 发放奖励
        string product_id = data.at("product_id");
        int qrCode = S2I(data.at("qrCode"));
        int goodsType = getPurchaseType(product_id);
        string json_data;
        if (goodsType == order::E_PURCHASE_LUCKPACK || goodsType == order::E_PURCHASE_SPECIALPACK ||
                goodsType == order::E_PURCHASE_GOLD  || goodsType == order::E_PURCHASE_INTEGRAL || goodsType == order::E_PURCHASE_RECHARGE)
        {
            mall::DispatchGoodsReq dispatchGoodsReq;
            dispatchGoodsReq.uid = req.uid;
            dispatchGoodsReq.QRCode = qrCode;
            mall::DispatchGoodsResp dispatchGoodsResp;
            iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid)->dispatchGoods(dispatchGoodsReq, dispatchGoodsResp);
            if (iRet != 0)
                THROW_LOGIC_ERROR("RPC dispatchGoods failed.");

            bool first_step = true;
            for (auto item : dispatchGoodsResp.data)
            {
                if (first_step)
                {
                    json_data += formatString("[{\"GoodsID\":{0},\"GoodsCount\":{1}}", toStringVec(item.GoodsID, item.GoodsCount));
                    first_step = false;
                }
                else
                {
                    json_data += formatString(", {\"GoodsID\":{0},\"GoodsCount\":{1}}", toStringVec(item.GoodsID, item.GoodsCount));
                }
            }

            json_data += "]";
        }
        else if (goodsType == order::E_PURCHASE_PIG)
        {
            goldpig::PurchaseGoldPigReq  purchaseGoldPigReq;
            purchaseGoldPigReq.uid = req.uid;
            goldpig::PurchaseGoldPigResp purchaseGoldPigResp;
            iRet = g_app.getOuterFactoryPtr()->getActivityServantPrx(req.uid)->purchase(purchaseGoldPigReq, purchaseGoldPigResp);
            if (iRet != 0 || purchaseGoldPigResp.resultCode != 0)
                THROW_LOGIC_ERROR("RPC goldPig purchase failed.");

            json_data = to_string(purchaseGoldPigResp.storage);
        }
        else
        {
            THROW_LOGIC_ERROR("invalid GoodsType:", goodsType);
        }

        rsp.resultCode = 0;
        rsp.json = formatString("{\"type\":{0},\"data\":{1}}", toStringVec(goodsType == order::E_PURCHASE_PIG ? order::E_PURCHASE_PIG : order::E_PURCHASE_GOLD, json_data));

        // 更改状态
        order_status = 3; //已经完成
        vector<map<string, string>> dataVec;
        make_write_data(dataVec, "order_status", dbagent::INT, order_status);

        dataproxy::TWriteDataRsp dataRsp;
        ROLLLOG_DEBUG << "writeProxyData orderNum : " << orderNum << endl;
        writeProxyData(orderNum, dataproxy::E_REDIS_WRITE, dataVec, dataRsp);
    }
    catch (const std::exception &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        iRet = -1;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}
