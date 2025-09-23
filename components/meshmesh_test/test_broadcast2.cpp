#include "meshmesh_test.h"
#include "defines.h"
#include "globals.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <meshsocket.h>
#include <string>

namespace esphome {
namespace meshmesh {

const std::string MeshmeshTest::broadcast2title = "Broadcast send/recvDatagram small packet";

void MeshmeshTest::broadcast2(){
    switch(mSubState){
        /**
         * Test 1: Send and receive small packet
         */
        case 0: 
            {
                STATE_LOGI(broadcast2title.c_str());
                mBuffer = new uint8_t[RX_BUFFER_SIZE];
                mSocket = new espmeshmesh::MeshSocket(TEST_PORT, espmeshmesh::MeshSocket::broadCastAddress);
                int16_t err = mSocket->open(espmeshmesh::MeshSocket::SOCK_DGRAM);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Socket opened", mState, mIndex == 1 ? 1 : 2);
                }
            }
        break;
        case 1: 
            { // Director: Send small datagram
                STATE_LOGI("Sending small datagram");
                int16_t err = mSocket->send((const uint8_t *)HELLO_STRING, HELLO_STRING_SIZE);
                ERR_CHECK(err) {
                    CHANGE_STATE_MSG("Small packet sent", mState, 3);
                }
            }
        break;
        case 2: 
            { // Others:Wait for remote hello
                int16_t rssi{0};
                int16_t res = mSocket->recvDatagram(mBuffer, RX_BUFFER_SIZE, mFrom, rssi);
                ERR_CHECK(res) {
                    if(res) {
                        CHANGE_STATE_MSG2("Received small datagram from %06X with rssi %d", mState, 4, mFrom, rssi);
                    } else {
                        TIMEOUT_CHECK(5000);
                    } 
                } 
            } 
            break;
        case 3: 
            { // Director: Wait reply
                int16_t rssi{0};
                int16_t res = mSocket->recvDatagram(mBuffer, RX_BUFFER_SIZE, mFrom, rssi);
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
                int16_t rssi{0};
                int16_t res = mSocket->recvDatagram(mBuffer, RX_BUFFER_SIZE, mFrom, rssi);
                ERR_CHECK(res) {
                    if(res > 0) {
                        CHANGE_STATE_MSG2("Received big packet request", mState, 8, mFrom, rssi);
                    } else {
                        TIMEOUT_CHECK(1000);
                    }
                }
            } 
        break;
        case 7: 
            { // Director: Wait big packet reply
                int16_t rssi{0};
                int16_t res = mSocket->recvDatagram(mBuffer, RX_BUFFER_SIZE, mFrom, rssi);
                ERR_CHECK(res) {
                    if(res > 0) {
                        CHANGE_STATE_MSG2("Received big packet reply", mState, 99, mFrom, rssi);
                    } else {
                        TIMEOUT_CHECK(1000);
                    }
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
        case 98:
            // End with error
            delete mBuffer;
            mBuffer = nullptr;
            delete mSocket;
            mSocket = nullptr;
            CHANGE_STATE(DONE, 0);
            STATE_LOGE2("End with error", "Big packet reply sent");
        break;        
        case 99:
            // End with success
            STATE_LOG2I("%s done", broadcast2title.c_str());
            delete[] mBuffer;
            mBuffer = nullptr;
            delete mSocket;
            mSocket = nullptr;
            CHANGE_STATE(UNICAST1, 0);
        break;
    }
   
}

}  // namespace meshmesh
}  // namespace esphome