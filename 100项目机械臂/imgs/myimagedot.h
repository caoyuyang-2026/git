#ifndef __MYIMAGEDOT_H__
#define __MYIMAGEDOT_H__

extern const unsigned char g_image_dot_dog_frame_0_32x32[128];
extern const unsigned char g_image_dot_dog_frame_1_32x32[128];
extern const unsigned char g_image_dot_dog_frame_2_32x32[128];
extern const unsigned char g_image_dot_dog_frame_3_32x32[128];
extern const unsigned char g_image_dot_dog_frame_4_32x32[128];
extern const unsigned char g_image_dot_dog_frame_5_32x32[128];
extern const unsigned char g_image_dot_dog_frame_6_32x32[128];
extern const unsigned char g_image_dot_dog_frame_7_32x32[128];
extern const unsigned char g_image_dot_dog_frame_8_32x32[128];
extern const unsigned char g_image_dot_dog_frame_9_32x32[128];
extern const unsigned char g_image_dot_dog_frame_10_32x32[128];
extern const unsigned char g_image_dot_dog_frame_11_32x32[128];

// 图像信息结构体
typedef struct {
    const char* name;              // 图像名字，方便索引指定图像数据
    const unsigned char* address;  // 图像数组入口地址
    unsigned int width;            // 图像宽度
    unsigned int height;           // 图像高度
    unsigned int size;             // 图像数组大小
} image_info_t;

static const image_info_t g_image_dot_tbl[12] = {//static
    {"dog_frame_0", g_image_dot_dog_frame_0_32x32, 32, 32, 128 },
    {"dog_frame_1", g_image_dot_dog_frame_1_32x32, 32, 32, 128 },
    {"dog_frame_2", g_image_dot_dog_frame_2_32x32, 32, 32, 128 },
    {"dog_frame_3", g_image_dot_dog_frame_3_32x32, 32, 32, 128 },
    {"dog_frame_4", g_image_dot_dog_frame_4_32x32, 32, 32, 128 },
    {"dog_frame_5", g_image_dot_dog_frame_5_32x32, 32, 32, 128 },
    {"dog_frame_6", g_image_dot_dog_frame_6_32x32, 32, 32, 128 },
    {"dog_frame_7", g_image_dot_dog_frame_7_32x32, 32, 32, 128 },
    {"dog_frame_8", g_image_dot_dog_frame_8_32x32, 32, 32, 128 },
    {"dog_frame_9", g_image_dot_dog_frame_9_32x32, 32, 32, 128 },
    {"dog_frame_10", g_image_dot_dog_frame_10_32x32, 32, 32, 128 },
    {"dog_frame_11", g_image_dot_dog_frame_11_32x32, 32, 32, 128 },
};

#endif //__MYIMAGEDOT_H__
