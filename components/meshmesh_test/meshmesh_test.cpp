#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "meshmesh_test.h"

#include <meshsocket.h>
#include <string>

namespace esphome {
namespace meshmesh {

static const char *TAG = "meshmesh_test.component";
static const uint16_t TEST_PORT = 0x11;
static const uint8_t RX_BUFFER_SIZE = 128;

void MeshmeshTest::setup() {
    mLastTime = millis();
}

void MeshmeshTest::loop() {
    switch(mState) {
        case STARTUP:
            stateStartup();
            break;
        case TEST1:
            break;
        case TEST2:
            break;
    }
}

void MeshmeshTest::dump_config(){
    ESP_LOGCONFIG(TAG, "Meshmesh test component");
    ESP_LOGCONFIG(TAG, "Index: %d", mIndex);
}

/**
 * Testm boradcast function
 */
void MeshmeshTest::stateStartup(){
    switch(mSubState){
        case 0:
            ESP_LOGI(TAG, "Testing broadcast functions %d", mIndex);
            mBuffer = new uint8_t[RX_BUFFER_SIZE];
            mSocket = new espmeshmesh::MeshSocket(TEST_PORT, espmeshmesh::MeshSocket::broadCastAddress);
            mSocket->open(espmeshmesh::MeshSocket::SOCK_DGRAM);
            mLastTime = millis();
            mSubState = mIndex == 1 ? 1 : 2;
            break;
        case 1: // Director: Wait 2 seconds and say hello
            if(millis() - mLastTime > 2000) {
                ESP_LOGI(TAG, "Sending hello");
                if(mIndex == 1) {
                     int16_t err = mSocket->send((const uint8_t *)"Hello1!!", 9);
                     if(err < 0) {
                        ESP_LOGE(TAG, "Error sending hello: %d", err);
                        mSubState = 99;
                     } else {
                        ESP_LOGI(TAG, "Hello sent");
                        mSubState = 3;
                     }
                }
            }
        case 2: { // Others:Wait for remote hello
            int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
            if(res < 0) {
                ESP_LOGE(TAG, "Procedue %d step %d error %d", mState, mSubState, res);
                mSubState = 99;
            } else if(res > 0) {
                ESP_LOGI(TAG, "Received hello: %s", mBuffer);
                mSubState = 4;
            }
            } break;
        case 3: { // Director: Wait reply
            int16_t res = mSocket->recv(mBuffer, RX_BUFFER_SIZE);
            if(res < 0) {
                ESP_LOGE(TAG, "Procedue %d step %d error %d", mState, mSubState, res);
                mSubState = 99;
            } else if(res > 0) {
                std::string reply((const char *)mBuffer, res);
                if(reply == "Hello1!!") {
                    ESP_LOGI(TAG, "Received reply: %s", reply.c_str());
                } else {
                    ESP_LOGE(TAG, "Received reply: %s", reply.c_str());
                }
                mSubState = 99;
            }
        } break;
        case 4: { // Others: Send reply
            int16_t err = mSocket->send((const uint8_t *)"Hi!!", 9);
            if(err < 0) {
                ESP_LOGE(TAG, "Error sending reply: %d", err);
            }
            mSubState = 99;
        } break;
        case 99: // Done
            ESP_LOGI(TAG, "Done");
            delete[] mBuffer;
            mBuffer = nullptr;
            mState = TEST1;
            mSubState = 0;
            break;
    }
}

}  // namespace meshmesh
}  // namespace esphome