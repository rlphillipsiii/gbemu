#include <cstring>
#include <thread>
#include <mutex>
#include <list>
#include <chrono>
#include <string>

#include "socketlink_windows.h"
#include "memorycontroller.h"
#include "interrupt.h"
#include "logging.h"
#include "configuration.h"
#include "memmap.h"

using std::thread;
using std::lock_guard;
using std::mutex;
using std::list;
using std::unique_lock;
using std::string;

SocketLink::SocketLink(MemoryController & memory)
    : ConsoleLink(memory)
{

}

void SocketLink::stop()
{

}
