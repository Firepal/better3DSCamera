/**
 *		Example by Nba_Yoh based on the one of smealum, thanks to him.
 */

#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>

#include <3ds.h>
#include <webp/encode.h>

#include "camera.h"

Handle threadExitRequest, y2rEvent;
Thread threadHandle;

#define STACKSIZE (4 * 1024)


#define WAIT_TIMEOUT 1000000000ULL
#define WIDTH 400
#define HEIGHT 240
#define SCREEN_SIZE WIDTH * HEIGHT * 2
#define BUF_SIZE SCREEN_SIZE * 2

void writeToFile(void* picBuf, size_t bufSize) {

}


void cleanup() {
	y2rExit();
	gfxExit();
}

int maein(int argc, char** argv)
{
    // Initializations
    gfxInitDefault();
    gfxSet3D(true);
    consoleInit(GFX_BOTTOM, NULL);

    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);

    Camera cam;


    struct timeval previous;
    gettimeofday(&previous, 0);
    struct timeval now;
    int count = 0;

    u8 *fb_L, *fb_R;

    Y2RU_SetTransferEndInterrupt(true);
    Y2RU_GetTransferEndEvent(&y2rEvent);    // to activate the event trigger


    // Main loop
    while (aptMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown();
        if(kDown & KEY_START)
            break; // break in order to return to hbmenu
        
        if (kDown & KEY_R) {

        }

        fb_L = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        fb_R = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);

        
        printf("entering critical section");

        cam.lock();
        // to send the YUV datas of the camera
        Y2RU_SetSendingYUYV((u8*) &cam.rawBuffer[0], cam.get_image_buffer_size(), cam.width * 2, 0);    
        cam.unlock();

        Y2RU_SetReceiving((void *) &cam.convBuffer[0], cam.get_image_buffer_size() * 3, cam.width * 2, 0);
        Y2RU_StartConversion();
        svcWaitSynchronization(y2rEvent, 1000 * 1000 * 20);

        float slider = osGet3DSliderState();
        cam.lock();
        printf("\ndata: %08X\n",cam.rawBuffer[5]);
        if (cam.updatedBuf) {
            cam.updatedBuf = false;
            cam.write_img_to(fb_L);  // draw to the framebuffer
        }
        cam.unlock();
        // if (slider > 0.0)  {
        //     cam.write_img_to(fb_R,cam.get_image_buffer_size());  // draw to the framebuffer
        // }

        gettimeofday(&now, 0);
        count++;

        if((int)((now.tv_sec)-(previous.tv_sec) > 0))
        {
            // printf("seconds: %d\n", count);
            count = 0;
            previous = now;

        }

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gspWaitForVBlank();
        gfxSwapBuffers();
    }

    cam.exit();
    cleanup();

    return 0;
}
