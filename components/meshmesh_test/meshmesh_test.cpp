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
                CHANGE_STATE_MSG("--------------------------------", BROADCAST1, 0);
            }
            break;
    }
}

void MeshmeshTest::dump_config(){
    ESP_LOGCONFIG(TAG, "Meshmesh test component");
    ESP_LOGCONFIG(TAG, "Index: %d", mIndex);
}

uint8_t MeshmeshTest::checksum(const uint8_t *buffer, uint16_t size) const {
    uint8_t sum = 0;
    for(uint16_t i = 0; i < size; i++) {
        sum += buffer[i];
    }
    return sum;
}


}  // namespace meshmesh
}  // namespace esphome