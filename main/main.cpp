#include "StatusLightHandler.h"
#include "LoadingAnimation.h"
#include <bits/this_thread_sleep.h>
#include "CommandHandler.h"

extern "C" void app_main() {
    StatusLightHandler* statusLightHandler = new StatusLightHandler();
    CommandHandler* commandHandler = new CommandHandler(*statusLightHandler);
    
    // FF0010010180002009C440000001AFFE
}