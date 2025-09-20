#include "meshmesh_test.h"
#include "defines.h"
#include "globals.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <meshsocket.h>
#include <string>

namespace esphome {
namespace meshmesh {

const std::string MeshmeshTest::broadcast1title = "Broadcast send/recv small packet";

/**
 * Testm boradcast function
 */
void MeshmeshTest::broadcast1(){
    switch(mSubState){
        /**
         * Test 1: Send and receive small packet
         */
        case 0: 
            {
                STATE_LOGI(broadcast1title.c_str());
                mBuffer = new uint8_t[RX_BUFFER_SIZE];
                mSocket = new espmeshmesh::MeshSocket(TEST_PORT, espmeshmesh::MeshSocket::broadCastAddress);
                int16_t err = mSocket->open(espmeshmesh::MeshSocket::SOCK_DGRAM);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Socket opened", mState, mIndex == 1 ? 1 : 2);
                }
            }
        break;
        case 1: // Director: Wait 2 seconds and say hello
            if(millis() - mLastTime > 2000) {
                STATE_LOGI("Sending small packet");
                int16_t err = mSocket->send((const uint8_t *)HELLO_STRING, HELLO_STRING_SIZE);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Small packet sent", mState, 3);
                }
            } 
            break;
        case 2: 
            { // Others:Wait for remote hello
                int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
                ERR_CHECK(res) {
                    if(res > 0) {
                        CHANGE_STATE_MSG("Received small packet", mState, 4);
                    } else TIMEOUT_CHECK(30000);
                } 
            } 
            break;
        case 3: 
            { // Director: Wait reply
                int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
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
            { // Others: Send reply
                STATE_LOGI("Sending hello reply");
                int16_t err = mSocket->send((const uint8_t *)REPLY_STRING, REPLY_STRING_SIZE);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Hello reply sent", mState, 6);
                }
            } 
        break;
        /**
         * Test 2: Send and receive big packet
         */
        case 5: 
            { // Director: Send big packet
                STATE_LOG2I("Sending big packet of size %d", SEND_BUFFER_SIZE);
                int16_t err = mSocket->send((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Big packet sent", mState, 7);
                }
            } 
        break;
        case 6: 
            { // Others: Wait for big packet
                int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
                ERR_CHECK(res) {
                    if(res > 0) {
                        STATE_LOG2I("Received big packet request of size %d", res);
                        CHANGE_STATE(mState, 8);
                    } else TIMEOUT_CHECK(1000);
                }
            } 
        break;
        case 7: 
            { // Director: Wait big packet reply
                int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
                ERR_CHECK(res) {
                    if(res > 0) {
                        STATE_LOG2I("Received big packet reply of size %d", res);
                        CHANGE_STATE(mState, 99);
                    } else TIMEOUT_CHECK(1000);
                }
            } 
        break;
        case 8:
            { // Others: Send big packet reply
                STATE_LOG2I("Sending big packet reply of size %d", SEND_BUFFER_SIZE);
                int16_t err = mSocket->send((const uint8_t *)LORE_IPSUM, SEND_BUFFER_SIZE);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Big packet reply sent", mState, 99);
                }
            } 
        break;
        case 99: // Done
            STATE_LOG2I("%s done", broadcast1title.c_str());
            delete[] mBuffer;
            mBuffer = nullptr;
            delete mSocket;
            mSocket = nullptr;
            CHANGE_STATE(BROADCAST2, 0);
            break;
    }
}

}  // namespace meshmesh
}  // namespace esphome