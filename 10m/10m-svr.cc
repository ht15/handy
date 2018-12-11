#include <handy/handy.h>

using namespace std;
using namespace handy;

struct Report {
    long connected;
    long closed;
    long recved;
    Report() { memset(this, 0, sizeof(*this)); }
};

int main(int argc, const char* argv[]) {
    if (argc < 5) {
        printf("usage: %s <begin port> <end port> <subprocesses> <management port>\n", argv[0]);
        return 1;
    }
    int begin_port = atoi(argv[1]);
    int end_port = atoi(argv[2]);
    int processes = atoi(argv[3]);
    int man_port = atoi(argv[4]);
    int pid = 1;
    for (int i = 0; i < processes; i ++) {
        pid = fork();
        if (pid == 0) { // a child process, break
            break;
        }
    }
    EventBase base;   // copy when write : the variables of  parent and child process is in different address when  writing
    if (pid == 0) { //child process
        usleep(100*1000); // wait master to listen management port
        vector<TcpServerPtr> svrs;
        long connected = 0, closed = 0, recved = 0;
        for (int i = 0; i < end_port-begin_port; i ++) {
            TcpServerPtr p = TcpServer::startServer(&base, "", begin_port + i, true);
            p->onConnCreate([&]{    // onConnCreate --> handleAccept().addcon (: create TcpConn to handle read and write)
                TcpConnPtr con(new TcpConn);
                con->onState([&](const TcpConnPtr& con) {   // poller.loop_once --> TcpConn.handleHandshake(if state changed)
                    auto st = con->getState();
                    if (st == TcpConn::Connected) {
                        connected ++;
                    } else if (st == TcpConn::Closed || st == TcpConn::Failed) {
                        closed ++;
                        connected --;
                    }
                });
                con->onMsg(new LengthCodec, [&](const TcpConnPtr& con, Slice msg) {   // --> OnRead(func = this lambda)
                    recved ++;
                    info("recv from client msg: %s", msg);
                    con->sendMsg(msg);
                });
                return con;
            });
            svrs.push_back(p);
        }
        TcpConnPtr report = TcpConn::createConnection(&base, "127.0.0.1", man_port, 3000);
        report->onMsg(new LineCodec, [&](const TcpConnPtr& con, Slice msg) {
            printf("recv from master:%s\n", msg);
            if (msg == "exit") {
                info("recv exit msg from master, so exit");
                base.exit();
            }
        });
        report->onState([&](const TcpConnPtr& con) {
            if (con->getState() == TcpConn::Closed) {
                base.exit();
            }
        });
        base.runAfter(100, [&]() {
            report->sendMsg(util::format("%d connected: %ld closed: %ld recved: %ld", getpid(), connected, closed, recved));
        }, 1000);
        base.loop();
    } else {
        map<int, Report> subs;
        TcpServerPtr master = TcpServer::startServer(&base, "127.0.0.1", man_port);
        master->onConnMsg(new LineCodec, [&](const TcpConnPtr& con, Slice msg) {  // TcpServer readcb can multi-allocation,
            printf("recv report msg1:%s\n", msg);
            auto fs = msg.split(' ');
            if (fs.size() != 7) {
                error("number of fields is %lu expected 7", fs.size());
                return;
            }
            Report& c = subs[atoi(fs[0].data())];
            c.connected = atoi(fs[2].data());
            c.closed = atoi(fs[4].data());
            c.recved = atoi(fs[6].data());
        });
        base.runAfter(3000, [&](){
            for(auto& s: subs) {
                Report r = s.second;
                printf("pid: %6d connected %6ld closed: %6ld recved %6ld\n", s.first, r.connected, r.closed, r.recved);
            }
            //printf("\n");
        }, 10000);
        /*
        // test for exit
        base.runAfter(6000,[&](){
            master->_conn->onMsg(new LineCodec, [&](const TcpConnPtr& con, Slice msg) {
                printf("recv report msg2:%s\n", msg);
                con->sendMsg("exit");
            });
        });
        */
       
        base.loop();
    }
    info("program exited");
}
