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
        case WAIT_START:
            wait_start();
            break;
        case BROADCAST1:
            broadcast1();
            break;
        case BROADCAST2:
            broadcast2();
            break;
        case UNICAST1:
            unicast1();
            break;
        case UNICAST2:
            unicast2();
            break;
        case DONE:
            if(millis() - mLastTime > 2500) {
                ESP_LOGI(TAG, "Test for node %d ended", mIndex);
                mLastTime = millis();
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

int16_t MeshmeshTest::openTestSocket() {
    closeTestResources();
    mBuffer = new uint8_t[RX_BUFFER_SIZE];
    mSocket = new espmeshmesh::MeshSocket(TEST_PORT);
    return mSocket->open();
}

void MeshmeshTest::closeTestResources() {
    delete[] mBuffer;
    mBuffer = nullptr;
    delete mSocket;
    mSocket = nullptr;
}

int16_t MeshmeshTest::sendTestDatagram(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &target) {
    return mSocket->sendDatagram(data, size, target, nullptr);
}

int16_t MeshmeshTest::recvTestDatagram() {
    return mSocket->recvDatagram(mBuffer, RX_BUFFER_SIZE, mFrom, mRssi);
}

espmeshmesh::MeshAddress MeshmeshTest::broadcastTarget() const {
    return espmeshmesh::MeshAddress(TEST_PORT, espmeshmesh::MeshAddress::broadCastAddress);
}

espmeshmesh::MeshAddress MeshmeshTest::unicastTarget() const {
    return espmeshmesh::MeshAddress(espmeshmesh::MeshAddress::SRC_UNICAST, TEST_PORT, mFrom.address);
}

uint16_t MeshmeshTest::afterOpenSubState() const {
    return IS_DIRECTOR() ? 1 : 2;
}

void MeshmeshTest::wait_start() {
    const uint32_t delay = IS_DIRECTOR() ? 2000 : 500;
    if(millis() - mLastTime > delay) {
        CHANGE_STATE_MSG2("Starting test for node %d", BROADCAST1, 0, mIndex);
    }
}

}  // namespace meshmesh
}  // namespace esphome
