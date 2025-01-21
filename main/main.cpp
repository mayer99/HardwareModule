#include "StatusLightHandler.h"
#include "LoadingAnimation.h"
#include <bits/this_thread_sleep.h>
#include "CommandHandler.h"

extern "C" void app_main() {
    StatusLightHandler* statusLightHandler = new StatusLightHandler();
    CommandHandler* commandHandler = new CommandHandler(*statusLightHandler);
    // ff f1 f2 f3 fe ff ff 00 10 01 01 80 00 20 09 c4 40 00 00 01 af fe ff fe 55 55
    // FF0010010180002009C440000001AFFE
}