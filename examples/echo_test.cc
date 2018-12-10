#include <handy/handy.h>
#include <sys/wait.h>

using namespace std;
using namespace handy;

int main(int argc, char const *argv[])
{
	EventBase base;
	auto con = TcpConn::createConnection(&base, "", 2099, 20*1000);

	base.runAfter(100, [&](){
        con->sendMsg("nihao");
    }, 50);
	
	base.loop();
}
