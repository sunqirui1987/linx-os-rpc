/**
 * xiaozhi-esp32 与 LinxOS RPC 集成示例
 * 
 * 这个文件展示了如何将 LinxOS RPC 客户端库集成到 xiaozhi-esp32 项目中，
 * 实现设备端作为 gRPC 客户端，允许远程服务器调用设备的各种功能。
 * 
 * 集成步骤：
 * 1. 包含 LinxOS RPC 头文件
 * 2. 创建设备信息和连接配置
 * 3. 初始化 RPC 客户端
 * 4. 注册设备功能（AddTool）
 * 5. 连接到远程服务器
 * 6. 启动服务循环
 */

#include "linxos_rpc/client.h"
#include <iostream>
#include <json/json.h>
#include <thread>
#include <chrono>

// 模拟 xiaozhi-esp32 的硬件接口
// 在实际集成中，这些应该是对真实硬件的调用
namespace xiaozhi {
namespace hardware {

class VoiceModule {
public:
    static bool speak(const std::string& text, float speed = 1.0, int volume = 80) {
        std::cout << "[语音模块] 播放: \"" << text << "\" (速度:" << speed << ", 音量:" << volume << ")" << std::endl;
        // 模拟播放时间
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
    }
    
    static bool setVolume(int volume) {
        std::cout << "[语音模块] 设置音量: " << volume << std::endl;
        return true;
    }
    
    static int getVolume() {
        return 80; // 模拟当前音量
    }
};

class DisplayModule {
public:
    static bool showExpression(const std::string& expression, int duration = 3000) {
        std::cout << "[显示模块] 显示表情: " << expression << " (持续:" << duration << "ms)" << std::endl;
        return true;
    }
    
    static bool showText(const std::string& text, int duration = 5000) {
        std::cout << "[显示模块] 显示文本: \"" << text << "\" (持续:" << duration << "ms)" << std::endl;
        return true;
    }
    
    static bool setBrightness(int brightness) {
        std::cout << "[显示模块] 设置亮度: " << brightness << std::endl;
        return true;
    }
};

class LightModule {
public:
    static bool setRGB(int r, int g, int b, int brightness = 100) {
        std::cout << "[灯光模块] 设置RGB: (" << r << "," << g << "," << b << ") 亮度:" << brightness << std::endl;
        return true;
    }
    
    static bool setMode(const std::string& mode, int speed = 5) {
        std::cout << "[灯光模块] 设置模式: " << mode << " 速度:" << speed << std::endl;
        return true;
    }
};

class AudioModule {
public:
    static bool playFile(const std::string& file_path, float volume = 1.0) {
        std::cout << "[音频模块] 播放文件: " << file_path << " 音量:" << volume << std::endl;
        return true;
    }
    
    static bool startRecord(int duration = 5) {
        std::cout << "[音频模块] 开始录音: " << duration << "秒" << std::endl;
        return true;
    }
    
    static bool stopAudio() {
        std::cout << "[音频模块] 停止音频" << std::endl;
        return true;
    }
};

class SystemModule {
public:
    static bool restart(int delay = 0) {
        std::cout << "[系统模块] 重启设备 (延迟:" << delay << "秒)" << std::endl;
        return true;
    }
    
    static bool reconnectWifi() {
        std::cout << "[系统模块] 重连WiFi" << std::endl;
        return true;
    }
    
    static std::string getSystemInfo() {
        return "{\"cpu_usage\":25.5,\"memory_usage\":60.2,\"temperature\":45.8,\"uptime\":3600,\"wifi_status\":\"connected\",\"free_heap\":102400}";
    }
};

} // namespace hardware
} // namespace xiaozhi

// LinxOS RPC 工具函数实现
namespace linxos_tools {

/**
 * 语音播放工具
 */
std::string voice_speak(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    std::string text = request.get("text", "").asString();
    float speed = request.get("speed", 1.0).asFloat();
    int volume = request.get("volume", 80).asInt();
    
    bool success = xiaozhi::hardware::VoiceModule::speak(text, speed, volume);
    
    response["success"] = success;
    response["message"] = success ? "语音播放成功" : "语音播放失败";
    response["duration"] = text.length() * 0.1; // 模拟播放时长
    
    return response.toStyledString();
}

/**
 * 音量控制工具
 */
std::string voice_volume(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    if (request.isMember("volume")) {
        // 设置音量
        int volume = request["volume"].asInt();
        bool success = xiaozhi::hardware::VoiceModule::setVolume(volume);
        response["success"] = success;
        response["message"] = success ? "音量设置成功" : "音量设置失败";
    } else {
        // 获取音量
        int volume = xiaozhi::hardware::VoiceModule::getVolume();
        response["success"] = true;
        response["volume"] = volume;
        response["message"] = "获取音量成功";
    }
    
    return response.toStyledString();
}

/**
 * 表情显示工具
 */
std::string display_expression(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    std::string expression = request.get("expression", "neutral").asString();
    int duration = request.get("duration", 3000).asInt();
    
    bool success = xiaozhi::hardware::DisplayModule::showExpression(expression, duration);
    
    response["success"] = success;
    response["message"] = success ? "表情显示成功" : "表情显示失败";
    
    return response.toStyledString();
}

/**
 * 文本显示工具
 */
std::string display_text(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    std::string text = request.get("text", "").asString();
    int duration = request.get("duration", 5000).asInt();
    
    bool success = xiaozhi::hardware::DisplayModule::showText(text, duration);
    
    response["success"] = success;
    response["message"] = success ? "文本显示成功" : "文本显示失败";
    
    return response.toStyledString();
}

/**
 * 灯光控制工具
 */
std::string light_control(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    int red = request.get("red", 0).asInt();
    int green = request.get("green", 0).asInt();
    int blue = request.get("blue", 0).asInt();
    int brightness = request.get("brightness", 100).asInt();
    
    bool success = xiaozhi::hardware::LightModule::setRGB(red, green, blue, brightness);
    
    response["success"] = success;
    response["message"] = success ? "灯光控制成功" : "灯光控制失败";
    
    return response.toStyledString();
}

/**
 * 灯光模式工具
 */
std::string light_mode(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    std::string mode = request.get("mode", "solid").asString();
    int speed = request.get("speed", 5).asInt();
    
    bool success = xiaozhi::hardware::LightModule::setMode(mode, speed);
    
    response["success"] = success;
    response["message"] = success ? "灯光模式设置成功" : "灯光模式设置失败";
    
    return response.toStyledString();
}

/**
 * 音频播放工具
 */
std::string audio_play(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    std::string file_path = request.get("file_path", "").asString();
    float volume = request.get("volume", 1.0).asFloat();
    
    bool success = xiaozhi::hardware::AudioModule::playFile(file_path, volume);
    
    response["success"] = success;
    response["message"] = success ? "音频播放成功" : "音频播放失败";
    response["audio_id"] = "audio_" + std::to_string(time(nullptr));
    
    return response.toStyledString();
}

/**
 * 系统信息工具
 */
std::string system_info(const std::string& params) {
    Json::Value response;
    
    std::string system_status = xiaozhi::hardware::SystemModule::getSystemInfo();
    Json::Value status_json;
    Json::Reader reader;
    
    if (reader.parse(system_status, status_json)) {
        response["success"] = true;
        response["message"] = "获取系统信息成功";
        response["system_status"] = status_json;
    } else {
        response["success"] = false;
        response["message"] = "获取系统信息失败";
    }
    
    return response.toStyledString();
}

/**
 * 系统重启工具
 */
std::string system_restart(const std::string& params) {
    Json::Value request, response;
    Json::Reader reader;
    
    if (!reader.parse(params, request)) {
        response["success"] = false;
        response["message"] = "参数解析失败";
        return response.toStyledString();
    }
    
    int delay = request.get("delay", 0).asInt();
    bool success = xiaozhi::hardware::SystemModule::restart(delay);
    
    response["success"] = success;
    response["message"] = success ? "系统重启命令已发送" : "系统重启失败";
    
    return response.toStyledString();
}

} // namespace linxos_tools

/**
 * xiaozhi-esp32 主应用类
 * 
 * 这个类展示了如何在 xiaozhi-esp32 的 application.cc 中
 * 集成 LinxOS RPC 客户端。
 */
class XiaozhiApplication {
public:
    XiaozhiApplication() 
        : device_id_("xiaozhi_" + std::to_string(time(nullptr)))
        , rpc_client_(nullptr)
    {
    }
    
    bool Initialize() {
        std::cout << "=== xiaozhi-esp32 LinxOS RPC 集成示例 ===" << std::endl;
        std::cout << "设备ID: " << device_id_ << std::endl;
        
        // 1. 创建设备信息
        auto device_info = linxos::rpc::CreateXiaozhiDeviceInfo(device_id_, "1.0.0");
        
        // 2. 创建连接配置
        auto config = linxos::rpc::CreateDefaultConfig("localhost:50051");
        config.heartbeat_interval_s = 30;
        config.max_retry_count = 5;
        
        // 3. 创建 RPC 客户端
        rpc_client_ = std::make_unique<linxos::rpc::LinxOSRPCClient>(device_info, config);
        
        // 4. 设置状态回调
        rpc_client_->SetStatusCallback([this](linxos::rpc::ConnectionStatus status, const std::string& message) {
            OnConnectionStatusChanged(status, message);
        });
        
        // 5. 注册设备功能工具
        RegisterTools();
        
        std::cout << "LinxOS RPC 客户端初始化完成" << std::endl;
        return true;
    }
    
    bool Start() {
        if (!rpc_client_) {
            std::cerr << "RPC 客户端未初始化" << std::endl;
            return false;
        }
        
        // 连接到远程服务器
        std::cout << "正在连接到远程服务器..." << std::endl;
        if (!rpc_client_->Connect()) {
            std::cerr << "连接到远程服务器失败" << std::endl;
            return false;
        }
        
        // 启动 RPC 服务
        std::cout << "启动 RPC 服务..." << std::endl;
        if (!rpc_client_->Start()) {
            std::cerr << "启动 RPC 服务失败" << std::endl;
            return false;
        }
        
        std::cout << "xiaozhi-esp32 设备已就绪，等待远程调用..." << std::endl;
        return true;
    }
    
    void Stop() {
        if (rpc_client_) {
            std::cout << "停止 RPC 服务..." << std::endl;
            rpc_client_->Stop();
            rpc_client_->Disconnect();
        }
        std::cout << "xiaozhi-esp32 设备已停止" << std::endl;
    }
    
    void Run() {
        // 主循环 - 在实际的 xiaozhi-esp32 中，这里会是设备的主要业务逻辑
        std::cout << "设备运行中... (按 Ctrl+C 退出)" << std::endl;
        
        while (true) {
            // 模拟设备的其他工作
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 检查 RPC 客户端状态
            if (rpc_client_ && rpc_client_->GetStatus() == linxos::rpc::ConnectionStatus::ERROR) {
                std::cout << "检测到连接错误，尝试重新连接..." << std::endl;
                rpc_client_->Connect();
            }
        }
    }

private:
    void RegisterTools() {
        std::cout << "注册设备功能工具..." << std::endl;
        
        // 注册语音相关工具
        rpc_client_->AddTool("voice_speak", linxos_tools::voice_speak);
        rpc_client_->AddTool("voice_volume", linxos_tools::voice_volume);
        
        // 注册显示相关工具
        rpc_client_->AddTool("display_expression", linxos_tools::display_expression);
        rpc_client_->AddTool("display_text", linxos_tools::display_text);
        
        // 注册灯光相关工具
        rpc_client_->AddTool("light_control", linxos_tools::light_control);
        rpc_client_->AddTool("light_mode", linxos_tools::light_mode);
        
        // 注册音频相关工具
        rpc_client_->AddTool("audio_play", linxos_tools::audio_play);
        
        // 注册系统相关工具
        rpc_client_->AddTool("system_info", linxos_tools::system_info);
        rpc_client_->AddTool("system_restart", linxos_tools::system_restart);
        
        auto tools = rpc_client_->GetRegisteredTools();
        std::cout << "已注册 " << tools.size() << " 个工具: ";
        for (const auto& tool : tools) {
            std::cout << tool << " ";
        }
        std::cout << std::endl;
    }
    
    void OnConnectionStatusChanged(linxos::rpc::ConnectionStatus status, const std::string& message) {
        const char* status_names[] = {"DISCONNECTED", "CONNECTING", "CONNECTED", "RECONNECTING", "ERROR"};
        std::cout << "[状态变化] " << status_names[static_cast<int>(status)] << ": " << message << std::endl;
    }

private:
    std::string device_id_;
    std::unique_ptr<linxos::rpc::LinxOSRPCClient> rpc_client_;
};

/**
 * 主函数 - 模拟 xiaozhi-esp32 的 main 函数
 */
int main() {
    try {
        XiaozhiApplication app;
        
        // 初始化应用
        if (!app.Initialize()) {
            std::cerr << "应用初始化失败" << std::endl;
            return -1;
        }
        
        // 启动应用
        if (!app.Start()) {
            std::cerr << "应用启动失败" << std::endl;
            return -1;
        }
        
        // 运行应用
        app.Run();
        
        // 停止应用
        app.Stop();
        
    } catch (const std::exception& e) {
        std::cerr << "应用运行异常: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}