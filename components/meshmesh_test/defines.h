#pragma once

#define STATE_LOGI(M) ESP_LOGI(TAG, "State %d step %d: %s", mState, mSubState, M)
#define STATE_LOG2I(M, ...) ESP_LOGI(TAG, "State %d step %d: " M, mState, mSubState, __VA_ARGS__)
#define STATE_LOGE(M) ESP_LOGE(TAG, "State %d step %d: %s", mState, mSubState, M)
#define STATE_LOGE2(M, ...) ESP_LOGE(TAG, "State %d step %d: " M, mState, mSubState, __VA_ARGS__)

#define ERR_CHECK(X) if(X < 0) { STATE_LOGE("Response error"); CHANGE_STATE(mState, 98); } else
#define CHANGE_STATE(X,Y) mState = X; mSubState = Y; mLastTime = millis()
#define CHANGE_STATE_MSG(M,X,Y) STATE_LOGI(M); CHANGE_STATE(X,Y)
#define CHANGE_STATE_MSG2(M,X,Y,...) STATE_LOG2I(M, __VA_ARGS__); CHANGE_STATE(X,Y)
#define CHANGE_STATE_MSGE(M,X,Y) STATE_LOGE(M); CHANGE_STATE(X,Y)
#define TIMEOUT_CHECK(X) if(millis() - mLastTime > X) { STATE_LOGE("Timeout reached"); CHANGE_STATE(mState, 98); }

