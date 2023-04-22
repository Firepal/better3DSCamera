#ifndef CAMERA_B3C_H
#define CAMERA_B3C_H
#include <3ds.h>

class Camera {
    volatile u8 selected_cameras = SELECT_OUT1_OUT2;

    Thread thread_receiving;
    LightLock lock_recvwrite;

    volatile bool threadQuit = false;

    static void _receiving(void *class_ptr);
    void init_y2ru();

    void print(const char *yea);

public:
    u16 width = 400;
    u16 height = 240;
    volatile u8 *rawBuffer, *convBuffer;
    volatile bool updatedBuf = false;

    Camera();
    u32 get_image_buffer_size();
    void init();
    void exit();
    void lock();
    void unlock();
    void write_img_to(void *dest);
};

#endif