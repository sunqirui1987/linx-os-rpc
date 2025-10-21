
---

## 5. 核心API模块

### 5.1 网络通信 RPC 驱动

> **模块说明**: 统一的网络通信接口，支持WiFi、HTTP、WebSocket等协议的RPC驱动注册  
> **芯片支持**: 🟢 ESP32 | 🟢 RK芯片 | 🟢 地平线 | 🟢 NV芯片

#### RPC 驱动注册映射

| RPC 接口 | 底层驱动函数 | 返回值 | 描述 | 芯片支持 |
|---------|-------------|--------|------|----------|
| `net_connect(config)` | `hal_network_connect` | `int` | 连接网络(WiFi/以太网)驱动注册 | 全平台 |
| `net_request(method, url, data, response)` | `hal_http_request` | `int` | HTTP请求驱动注册 | 全平台 |




### 5.3 音频系统 RPC 驱动

> **模块说明**: 音频录制、播放、处理的RPC驱动注册接口  
> **芯片支持**: 🟢 ESP32 | 🟢 RK芯片 | 🟢 地平线 | 🟢 NV芯片  
> **驱动差异**: ESP32基础音频驱动注册，RK芯片多媒体音频驱动，地平线AI音频驱动，NV芯片高性能音频驱动

#### 音频流类型定义

```c
// 音频格式枚举
typedef enum {
    AUDIO_FORMAT_PCM_16BIT = 0,    // PCM 16位
    AUDIO_FORMAT_PCM_24BIT,        // PCM 24位
    AUDIO_FORMAT_PCM_32BIT,        // PCM 32位
    AUDIO_FORMAT_OPUS,             // Opus压缩格式
    AUDIO_FORMAT_AAC,              // AAC压缩格式
    AUDIO_FORMAT_MP3               // MP3压缩格式
} audio_format_t;

// 音频流回调函数类型
typedef void (*audio_callback_t)(const uint8_t* data, size_t length, audio_format_t format);
```

#### 核心API

| 函数 | 参数 | 返回值 | 描述 | 芯片支持 |
|------|------|--------|------|----------|
| `audio_init()` | `void` | `int` | 初始化音频系统 | 全平台 |
| `audio_play(source，type)` | `const uint8* source, int type` | `int` | 播放音频(文件路径或URL或pcm，opus) | 全平台 |
| `audio_record(filename, duration)` | `const char* filename, int duration` | `int` | 录音到设备文件，指定时长(秒) | 全平台 |
| `audio_volume(level)` | `int level` | `int` | 设置音量(0-100)，-1获取当前音量 | 全平台 |



### 5.4 视觉模块 RPC 驱动

> **模块说明**: 摄像头硬件控制和图像AI分析的RPC驱动注册接口  
> **芯片支持**: 🔴 ESP32 | 🟢 RK芯片 | 🟢 地平线 | 🟢 NV芯片  
> **驱动差异**: ESP32支持基础拍照驱动，RK芯片及以上支持视频流驱动，地平线及以上支持AI识别驱动

#### 摄像头硬件API

| 函数 | 参数 | 返回值 | 描述 | 芯片支持 |
|------|------|--------|------|----------|
| `camera_init(width, height, fps)` | `int width, int height, int fps` | `camera_handle_t*` | 初始化摄像头 | 全平台 |
| `camera_capture(handle, callback)` | `camera_handle_t* handle, capture_callback_t callback` | `int` | 拍照捕获 | 全平台 |


**数据结构**:
```c
typedef struct {
    void* data;
    int size;
    int width, height;
} image_data_t;

typedef struct {
    char name[32];
    float confidence;
    int x, y, w, h;
} detection_result_t;

// 回调函数
typedef void (*capture_callback_t)(int result, image_data_t* image);
typedef void (*stream_callback_t)(image_data_t* frame);
typedef void (*detect_callback_t)(detection_result_t* results, int count);
typedef void (*face_callback_t)(detection_result_t* faces, int count);
typedef void (*scene_callback_t)(const char* scene_type, int people_count);
```

**RPC驱动注册示例**:
```c
// 1. 摄像头驱动注册
int hal_camera_init(int width, int height, int fps) {
    // 底层摄像头初始化
    return camera_hardware_init(width, height, fps);
}

int hal_camera_capture(capture_callback_t callback) {
    // 底层拍照实现
    return camera_hardware_capture(callback);
}

// 注册摄像头RPC驱动
void register_camera_drivers(void) {
    linx_rpc_register_driver("vision.cameraInit", 
                            "hal_camera_init",
                            (void*)hal_camera_init,
                            "RK,Horizon,NVIDIA");
                            
    linx_rpc_register_driver("vision.capture",
                            "hal_camera_capture", 
                            (void*)hal_camera_capture,
                            "RK,Horizon,NVIDIA");
}

// 2. AI推理驱动注册 (地平线及以上)
int hal_object_detection(image_data_t* image, detection_result_t* results) {
    // 底层AI推理实现
    return ai_inference_detect(image, results);
}

// 注册AI推理RPC驱动
void register_ai_drivers(void) {
    linx_rpc_register_driver("vision.detectObjects",
                            "hal_object_detection",
                            (void*)hal_object_detection,
                            "Horizon,NVIDIA");
}

// 3. RPC调用示例
void rpc_call_example(void) {
    // 通过RPC调用摄像头驱动
    void* driver_func = linx_rpc_find_driver("vision.capture");
    if (driver_func) {
        ((int(*)(capture_callback_t))driver_func)(on_photo_taken);
    }
}
```


### 5.6 任务管理===》闹钟功能。CURD （ setEvent（eventtype， time， audio）  ） 通知功能（notify（eventtype））

> **模块说明**: 统一的任务管理接口，支持任务调度、闹钟、备忘录、定时器等功能  
> **芯片支持**: 🟢 ESP32 | 🟢 RK芯片 | 🟢 地平线 | 🟢 NV芯片  
> **功能差异**: ESP32基础任务调度，RK芯片及以上支持多进程，地平线及以上支持优先级调度，NV芯片支持智能资源分配



### 5.7 系统工具

> **模块说明**: 统一的系统工具接口，提供状态查询、系统调用、信息获取等功能  
> **芯片支持**: 🟢 ESP32 | 🟢 RK芯片 | 🟢 地平线 | 🟢 NV芯片  
> **功能差异**: ESP32基础状态查询，RK芯片及以上支持详细系统信息，地平线及以上支持性能监控，NV芯片支持智能诊断

#### 核心API (仅3个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片支持 |
|------|------|--------|------|----------|
| `system_query(type, info)` | `int type, void* info` | `int` | 查询系统信息，电池/内存/存储/硬件/闹钟/备忘录 | 全平台 |
| `system_control(cmd, value)` | `int cmd, int value` | `int` | 系统控制，重启/休眠/亮度 | 全平台 |
| `system_call(command, args, result)` | `const char* command, const char* args, syscall_result_t* result` | `int` | 执行系统调用 | 全平台 |

**数据结构**:
```c
// 查询类型
#define QUERY_BATTERY    0  // 电池电量
#define QUERY_MEMORY     1  // 内存信息
#define QUERY_STORAGE    2  // 存储信息
#define QUERY_HARDWARE   3  // 硬件信息
#define QUERY_NETWORK    4  // 网络信息

// 控制命令
#define CTRL_REBOOT      0  // 重启
#define CTRL_SLEEP       1  // 休眠
#define CTRL_BRIGHTNESS  2  // 亮度
```

**RPC驱动注册示例**:
```c
// 底层系统查询驱动
int hal_battery_query(int* battery_level) {
    // 底层电池查询实现
    *battery_level = hardware_get_battery_level();
    return 0;
}

int hal_system_control(int cmd, int value) {
    // 底层系统控制实现
    switch(cmd) {
        case CTRL_REBOOT:
            hardware_reboot();
            break;
        case CTRL_SLEEP:
            hardware_sleep(value);
            break;
    }
    return 0;
}

// 注册系统工具RPC驱动
void register_system_drivers(void) {
    linx_rpc_register_driver("system.queryBattery", 
                            "hal_battery_query",
                            (void*)hal_battery_query,
                            "ESP32,RK,Horizon,NVIDIA");
                            
    linx_rpc_register_driver("system.control",
                            "hal_system_control", 
                            (void*)hal_system_control,
                            "ESP32,RK,Horizon,NVIDIA");
}

// RPC调用示例
void system_rpc_example(void) {
    // 通过RPC查询电池电量
    void* query_func = linx_rpc_find_driver("system.queryBattery");
    if (query_func) {
        int battery;
        ((int(*)(int*))query_func)(&battery);
        printf("电池电量: %d%%\n", battery);
    }
}
```


