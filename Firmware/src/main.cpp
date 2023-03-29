#include <Arduino.h>
#include "log.h"
#include "ble.h"
#include "global.h"
#include "service.h"
#include "capture.h"
#include "tf.h"
#include "task.h"
#include "config.h"
void setup()
{
    LOG_INIT(921600);
    USBSerial.setDebugOutput(false);
    tf_unmount();
    global_init();
    #ifdef BT_ENABLED
    ble_init();
    #endif
    service_init();
    capture_init();
    task_init();
}
void loop()
{
    global_run();
    #ifdef BT_ENABLED
    ble_run();
    #endif
    service_run();
    task_run();
    capture_run();
}