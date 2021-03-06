#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    HSHAPtr hsha = HSHA::startServer(&base, "", 2099, 2);
    exitif(!hsha, "bind failed");
    Signal::signal(SIGINT, [&, hsha]{ base.exit(); hsha->exit(); signal(SIGINT, SIG_DFL);});

    hsha->onMsg(new LineCodec, [](const TcpConnPtr& con, const string& input){
        int ms = rand() % 1000;
        info("processing a msg");
        usleep(ms * 1000);
        return util::format("%s used %d ms", input.c_str(), ms);
    });
    // for (int i = 0; i < 1; i ++) {
    //     TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
    //     con->onMsg(new LineCodec, [](const TcpConnPtr& con, Slice msg) {
    //         info("%.*s recved", (int)msg.size(), msg.data());
    //         //con->close();
    //         con->sendMsg("hello");
    //     });
    //     con->onState([](const TcpConnPtr& con) {  // when connect , except handleAccept  also handle a read operation
    //         if (con->getState() == TcpConn::Connected) {
    //             con->sendMsg("hello");
    //             printf("HsHa onState handread\n");
    //         }
    //     });
    // }
    base.runAfter(10000, [&, hsha]{base.exit(); hsha->exit(); });
    base.loop();
    info("program exited");
}
