#include "meshmesh_test.h"
#include "defines.h"
#include "globals.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <meshsocket.h>
#include <string>

namespace esphome {
namespace meshmesh {

void MeshmeshTest::setup() {
    mLastTime = millis();
}
 
void MeshmeshTest::loop() {

    switch(mState) {
        case BROADCAST1:
            broadcast1();
            break;
        case BROADCAST2:
            broadcast2();
            break;
        case DONE:
            if(millis() - mLastTime > 500) {
                CHANGE_STATE_MSG("Test 1 timeout", BROADCAST1, 0);
            }
            break;
    }
}

void MeshmeshTest::dump_config(){
    ESP_LOGCONFIG(TAG, "Meshmesh test component");
    ESP_LOGCONFIG(TAG, "Index: %d", mIndex);
}



}  // namespace meshmesh
}  // namespace esphome