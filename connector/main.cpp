#include "Connector.h"
#include <QtWidgets/QApplication>
#include "asio/tool/util.h"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

using namespace boost::asio;
using namespace ningto;
int main(int argc, char *argv[])
{
    setting::initlog(argv[0], true);

    QApplication a(argc, argv);
    LOG(INFO) << "connector start";
    Connector w;
    w.show();
    return a.exec();
}