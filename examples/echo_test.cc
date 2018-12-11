#include <handy/handy.h>
#include <sys/wait.h>

using namespace std;
using namespace handy;

int main(int argc, char const *argv[])
{
	EventBase base;
	TcpConnPtr report = TcpConn::createConnection(&base, "127.0.0.1", 2099, 3000);
    report->onMsg(new LineCodec, [&](const TcpConnPtr& con, Slice msg) {
        if (msg == "exit") {
            info("recv exit msg from master, so exit");
            base.exit();
        }
        printf("recv server:%s\n", msg);
    });
    report->onState([&](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Closed) {
            base.exit();
        }
    });
    base.runAfter(100, [&]() {
        report->sendMsg("test");
    }, 5000);
    base.loop();
}
