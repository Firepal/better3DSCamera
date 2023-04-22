#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <functional>
#include "camera.h"

#define WAIT_TIMEOUT 1000000000ULL

Camera::Camera() {
    this->init();
}


void test(void*arg) {
    printf("wtf");
    Camera *cam = (Camera*)arg;
    cam->lock();
    cam->unlock();
}

void Camera::init() {
    camInit();
    LightLock_Init(&lock_recvwrite);

    // one buffer used for both eyes
    rawBuffer = (u8*) malloc((width * height) * 2 /* bytes*/ * 2 /*images*/);
    convBuffer = (u8*) malloc((width * height) * 3);

    camInit();

    CAMU_SetSize(selected_cameras, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(selected_cameras, OUTPUT_YUV_422, CONTEXT_A);

    // TODO: For some reason frame grabbing times out above 10fps. Figure out why this is.
    CAMU_SetFrameRate(selected_cameras, FRAME_RATE_30_TO_10);

    CAMU_SetNoiseFilter(selected_cameras, false);
    CAMU_SetAutoExposure(selected_cameras, true);
    CAMU_SetAutoWhiteBalance(selected_cameras, true);

    CAMU_SetTrimming(PORT_CAM1, false);
    CAMU_SetTrimming(PORT_CAM2, false);

    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    printf("made thread");
    thread_receiving = threadCreate(Camera::_receiving, (void*)this, (8 * 1024), prio-1, -2, false);
}

void Camera::exit() {
    free((void*) rawBuffer);
    free((void*) convBuffer);
    camExit();
    // svcSignalEvent(threadExitRequest);
}

u32 Camera::get_image_buffer_size() {
    return width * height * 2;
}

void Camera::print(const char *yea) {
    printf(yea);
}

void Camera::lock() {
    printf("lock");
    LightLock_Lock(&lock_recvwrite);
}

void Camera::unlock() {
    printf("unlock");
    LightLock_Unlock(&lock_recvwrite);
}

void Camera::write_img_to(void *dest) {
    u8 *dest_8 = (u8*)dest;
    u8 *img_8 = (u8*)convBuffer;

    dest_8+= height*3 - 8*3;
    for(int j = 0; j < height/8; j++){
        dest_8 -= height*width*3 + 8*3;
        for(int i = 0; i < width; i++){
                img_8+=3*8;
                dest_8+=height*3;
                memcpy(dest_8, img_8, 1*8*3);
            }
    }
}

void Camera::_receiving(void *class_ptr) {
    Camera* cam = (Camera*)(class_ptr);
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);

    if(!cam->rawBuffer) {
        cam->print("Failed to allocate memory!");
        svcExitThread();
    }

    u32 bufSize;
    CAMU_GetMaxBytes(&bufSize, cam->width, cam->height);
    CAMU_SetTransferBytes(PORT_BOTH, bufSize, cam->width, cam->height);

    CAMU_Activate(SELECT_OUT1_OUT2);

    CAMU_ClearBuffer(PORT_BOTH);
    CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);

    CAMU_StartCapture(PORT_BOTH);
    CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_MOVIE);

    Handle camReceiveEvent = 0;
    Handle camReceiveEvent2 = 0;
    Handle camErrorBuf = 0;
    printf("yea");

    while(!cam->threadQuit)
    {
        cam->lock();
        u32 screen_size = cam->width * cam->height * 2;
        CAMU_SetReceiving(&camReceiveEvent, (void*) cam->rawBuffer, PORT_CAM1, screen_size, (s16) bufSize);
        CAMU_SetReceiving(&camReceiveEvent2, (void*)(cam->rawBuffer + screen_size), PORT_CAM2, screen_size, (s16) bufSize);

        CAMU_GetBufferErrorInterruptEvent(&camErrorBuf,PORT_BOTH);

        Handle evts[2] = {camReceiveEvent,camReceiveEvent2};
        s32 index = 0;
        if (svcWaitSynchronizationN(&index, evts, 2, false, WAIT_TIMEOUT)) {
            cam->print("timedout\n");

        } else {
            cam->print("got frame?\n");
            cam->updatedBuf = true;
        };
        cam->unlock();

        Result error = svcWaitSynchronization(camErrorBuf,WAIT_TIMEOUT);
        if (error) {
            printf("\n0x%08X\n",(error));
        }

        svcCloseHandle(camReceiveEvent);
        svcCloseHandle(camReceiveEvent2);
        svcCloseHandle(camErrorBuf);
    }

    printf("CAMU_StopCapture: 0x%08X\n", (unsigned int) CAMU_StopCapture(PORT_BOTH));

    printf("CAMU_Activate: 0x%08X\n", (unsigned int) CAMU_Activate(SELECT_NONE));

    // threadExit(0);
}