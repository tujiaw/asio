#include <iostream>
#include "asio/proto/pb_simple.pb.h"
#include "asio/msgclient.h"
#include "asio/msgserver.h"
#include "asio/msgbus.h"
#include "asio/tool/util.h"
#include <thread>

using namespace ningto;

class BaseHandler : public MsgHandler
{
public:
    void advise(MsgServer *server) override
    {
        ADVISE_HANDLER(ProtoSimple::PingPongReq, ProtoSimple::PingPongRsp, PingPong);
        ADVISE_HANDLER(ProtoSimple::DaytimeReq, ProtoSimple::DaytimeRsp, Daytime);
        ADVISE_HANDLER(ProtoSimple::DiscardReq, ProtoSimple::DiscardRsp, Discard);
    }

private:
    void PingPong(ProtoSimple::PingPongReq *req, ProtoSimple::PingPongRsp *rsp)
    {
        rsp->set_content(req->content());
    }

    void Daytime(ProtoSimple::DaytimeReq *req, ProtoSimple::DaytimeRsp *rsp)
    {
        rsp->set_time(util::getFormatTime(util::currentMillisecond()));
    }

    void Discard(ProtoSimple::DiscardReq *req, ProtoSimple::DiscardRsp *rsp)
    {
        static std::atomic<int> s_total{ 0 };
        s_total += req->content().size();
        rsp->set_size(req->content().size());
        rsp->set_total(s_total);
    }
};

int main(int argc, char* argv[])
{
    setting::initlog(argv[0], true);
    setting::initEnableCompressSize();

    LOG(INFO) << "server start...";
    MsgServer server(5577);
    server.addMethodHanlder(std::make_shared<BaseHandler>());
    server.run();
    LOG(INFO) << "server exit!!!";

    //MsgBus bus(5588);
    //bus.run();

    return 0;
}