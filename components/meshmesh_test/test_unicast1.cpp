#include "meshmesh_test.h"
#include "defines.h"
#include "globals.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <meshsocket.h>
#include <string>
#include <functional>

namespace esphome {
namespace meshmesh {

const std::string MeshmeshTest::unicast1title = "Unicast sendDatagram/recvDatagram small packet";

void MeshmeshTest::unicast1(){
    switch(mSubState){
        case 0:
            {
                STATE_LOGI(unicast1title.c_str());
                int16_t err = openTestSocket();
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Socket opened", mState, afterOpenSubState());
                }
            }
        break;
        case 1:
            if(millis() - mLastTime > 2000) {
                STATE_LOGI("Sending small packet");
                int16_t err = sendTestDatagram((const uint8_t *)HELLO_STRING, HELLO_STRING_SIZE, unicastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Small packet sent", mState, 3);
                }
            }
            break;
        case 2:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        uint8_t ck1 = checksum(mBuffer, res);
                        uint8_t ck2 = checksum((const uint8_t *)HELLO_STRING, HELLO_STRING_SIZE);
                        if(ck1 == ck2) {
                            CHANGE_STATE_MSG("Received correct small packet", mState, 4);
                        } else {
                            CHANGE_STATE_MSGE("Received wrong small packet", mState, 99);
                        }
                    } else TIMEOUT_CHECK(30000);
                }
            }
            break;
        case 3:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        uint8_t ck1 = checksum(mBuffer, res);
                        uint8_t ck2 = checksum((const uint8_t *)REPLY_STRING, REPLY_STRING_SIZE);
                        if(ck1 == ck2) {
                            CHANGE_STATE_MSG("Received correct small packet reply", mState, 5);
                        } else {
                            CHANGE_STATE_MSGE("Received wrong small packet reply", mState, 99);
                        }
                    } else TIMEOUT_CHECK(1000);
                }
            }
            break;
        case 4:
            {
                STATE_LOGI("Sending hello reply");
                int16_t err = sendTestDatagram((const uint8_t *)REPLY_STRING, REPLY_STRING_SIZE, unicastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Hello reply sent", mState, 6);
                }
            }
        break;
        case 5:
            {
                STATE_LOG2I("Sending big packet of size %d", SEND_BUFFER_SIZE);
                int16_t err = sendTestDatagram((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE, unicastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Big packet sent", mState, 7);
                }
            }
        break;
        case 6:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        STATE_LOG2I("Received big packet request of size %d", res);
                        CHANGE_STATE(mState, 8);
                    } else TIMEOUT_CHECK(1000);
                }
            }
        break;
        case 7:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        STATE_LOG2I("Received big packet reply of size %d", res);
                        CHANGE_STATE(mState, 99);
                    } else TIMEOUT_CHECK(1000);
                }
            }
        break;
        case 8:
            {
                STATE_LOG2I("Sending big packet reply of size %d", SEND_BUFFER_SIZE);
                int16_t err = sendTestDatagram((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE, unicastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Big packet reply sent", mState, 99);
                }
            }
        break;
        case 98:
            closeTestResources();
            CHANGE_STATE(DONE, 0);
            STATE_LOGE2("End with error %s", unicast1title.c_str());
        break;
        case 99:
            closeTestResources();
            CHANGE_STATE(UNICAST2, 0);
            STATE_LOG2I("%s done", unicast1title.c_str());
            STATE_LOG2I("Free heap: %ld after", esp_get_free_heap_size());
            break;
    }
}

}  // namespace meshmesh
}  // namespace esphome
