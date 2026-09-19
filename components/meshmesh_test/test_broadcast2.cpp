#include "meshmesh_test.h"
#include "defines.h"
#include "globals.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <meshsocket.h>
#include <string>

namespace esphome {
namespace meshmesh {

const std::string MeshmeshTest::broadcast2title = "Broadcast sendDatagram/recvDatagram small packet";

void MeshmeshTest::broadcast2(){
    switch(mSubState){
        case 0:
            {
                STATE_LOGI(broadcast2title.c_str());
                int16_t err = openTestSocket();
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Socket opened", mState, afterOpenSubState());
                }
            }
        break;
        case 1:
            {
                STATE_LOGI("Sending small datagram");
                int16_t err = sendTestDatagram((const uint8_t *)HELLO_STRING, HELLO_STRING_SIZE, broadcastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Small packet sent", mState, 3);
                }
            }
        break;
        case 2:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res) {
                        CHANGE_STATE_MSG2("Received small datagram from %06lX with rssi %d", mState, 4, mFrom.address, mRssi);
                    } else {
                        TIMEOUT_CHECK(5000);
                    }
                }
            }
            break;
        case 3:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        std::string reply((const char *)mBuffer, res);
                        if(reply.compare(REPLY_STRING) == 0) {
                            CHANGE_STATE_MSG("Received hello reply", mState, 5);
                        } else {
                            STATE_LOGE("Received wrong hello reply");
                            CHANGE_STATE(mState, 5);
                        }
                    } else TIMEOUT_CHECK(1000);
                }
            }
            break;
        case 4:
            {
                STATE_LOGI("Sending hello reply");
                int16_t err = sendTestDatagram((const uint8_t *)REPLY_STRING, REPLY_STRING_SIZE, broadcastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Hello reply sent", mState, 6);
                }
            }
        break;
        case 5:
            {
                STATE_LOG2I("Sending big packet of size %d", SEND_BUFFER_SIZE);
                int16_t err = sendTestDatagram((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE, broadcastTarget());
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
                        CHANGE_STATE_MSG2("Received big packet request from %06lX with rssi %d", mState, 8, mFrom.address, mRssi);
                    } else {
                        TIMEOUT_CHECK(1000);
                    }
                }
            }
        break;
        case 7:
            {
                int16_t res = recvTestDatagram();
                ERR_CHECK(res) {
                    if(res > 0) {
                        CHANGE_STATE_MSG2("Received big packet reply from %06lX with rssi %d", mState, 99, mFrom.address, mRssi);
                    } else {
                        TIMEOUT_CHECK(1000);
                    }
                }
            }
        break;
        case 8:
            {
                STATE_LOG2I("Sending big packet reply of size %d", SEND_BUFFER_SIZE);
                int16_t err = sendTestDatagram((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE, broadcastTarget());
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Big packet reply sent", mState, 99);
                }
            }
        break;
        case 98:
            closeTestResources();
            CHANGE_STATE(DONE, 0);
            STATE_LOGE2("End with error %s", broadcast2title.c_str());
        break;
        case 99:
            STATE_LOG2I("%s done", broadcast2title.c_str());
            closeTestResources();
            CHANGE_STATE(UNICAST1, 0);
        break;
    }

}

}  // namespace meshmesh
}  // namespace esphome
