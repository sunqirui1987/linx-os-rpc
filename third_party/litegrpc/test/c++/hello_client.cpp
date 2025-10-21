/**
 * @file hello_client.cpp
 * @brief 设备端 Hello 服务提供者实现（简化版本）
 * 
 * 这个文件实现了设备端的 Hello 服务提供者，演示了"反向gRPC"模式：
 * - C++设备端连接到Go云端服务器
 * - 设备端注册Hello服务能力
 * - 云端调用设备端的Hello能力
 * - 设备端响应云端的调用请求
 * 
 * 架构说明：
 * - 设备端（C++）：服务提供者，主动连接云端，注册服务能力
 * - 云端（Go）：服务调用者，接受设备连接，调用设备能力
 * 
 * 这种模式适用于设备没有固定IP地址，但需要向云端提供服务的场景。
 * 
 * @author LiteGRPC Team
 * @date 2024
 */

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <cstring>
#include <functional>
#include <map>
#include <atomic>
#include <csignal>
#include <ctime>
#include <sstream>

// 简化的JSON处理（避免依赖外部库）
class SimpleJson {
public:
    std::map<std::string, std::string> data;
    
    void set(const std::string& key, const std::string& value) {
        data[key] = "\"" + value + "\"";
    }
    
    void set(const std::string& key, int value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, bool value) {
        data[key] = value ? "true" : "false";
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) oss << ",";
            oss << "\"" << pair.first << "\":" << pair.second;
            first = false;
        }
        oss << "}";
        return oss.str();
    }
    
    static std::string extractValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos += searchKey.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (pos >= json.length()) return "";
        
        if (json[pos] == '"') {
            // 字符串值
            pos++;
            size_t end = json.find('"', pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        } else {
            // 数字或布尔值
            size_t end = pos;
            while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ' ') {
                end++;
            }
            return json.substr(pos, end - pos);
        }
    }
};

// 工具函数类型定义
using ToolFunction = std::function<std::string(const std::string&)>;

/**
 * @brief 设备端连接状态枚举
 */
enum class DeviceConnectionStatus {
    DISCONNECTED = 0,  ///< 未连接
    CONNECTING = 1,    ///< 连接中
    CONNECTED = 2,     ///< 已连接
    RECONNECTING = 3,  ///< 重连中
    ERROR = 4          ///< 错误状态
};

/**
 * @brief 设备端 Hello 工具实现
 * 
 * 这个函数处理来自云端的 SayHello 调用请求，模拟设备端的 Hello 能力。
 * 
 * @param params JSON格式的参数字符串，包含name和message字段
 * @return JSON格式的响应字符串，包含reply和status字段
 */
std::string hello_tool_function(const std::string& params) {
    std::cout << "\n[设备端] 收到 Hello 工具调用请求: " << params << std::endl;
    
    // 解析请求参数（简化版本）
    std::string name = SimpleJson::extractValue(params, "name");
    std::string message = SimpleJson::extractValue(params, "message");
    
    if (name.empty()) name = "Unknown";
    
    // 模拟设备端处理 Hello 请求
    std::cout << "[设备端] 处理 Hello 请求 - Name: " << name << ", Message: " << message << std::endl;
    
    // 构造设备端回复
    std::string reply = "设备端 Hello 响应: Hello " + name + "! 设备已收到你的消息: " + message;
    
    // 创建成功响应
    SimpleJson response;
    response.set("success", true);
    response.set("reply", reply);
    response.set("status", 0);  // 0 表示成功
    response.set("device_id", "hello_device_001");
    response.set("timestamp", static_cast<int>(std::time(nullptr)));
    
    std::cout << "[设备端] 发送响应: " << reply << std::endl;
    return response.toString();
}

/**
 * @brief 连接状态回调函数
 * 
 * 当设备端与云端的连接状态发生变化时被调用
 * 
 * @param status 新的连接状态
 */
void OnConnectionStatusChanged(DeviceConnectionStatus status) {
    switch (status) {
        case DeviceConnectionStatus::DISCONNECTED:
            std::cout << "[设备端] 连接状态: 未连接" << std::endl;
            break;
        case DeviceConnectionStatus::CONNECTING:
            std::cout << "[设备端] 连接状态: 连接中..." << std::endl;
            break;
        case DeviceConnectionStatus::CONNECTED:
            std::cout << "[设备端] 连接状态: 已连接到云端" << std::endl;
            break;
        case DeviceConnectionStatus::RECONNECTING:
            std::cout << "[设备端] 连接状态: 重连中..." << std::endl;
            break;
        case DeviceConnectionStatus::ERROR:
            std::cout << "[设备端] 连接状态: 连接错误" << std::endl;
            break;
    }
}

/**
 * @brief 设备端 Hello 服务提供者类
 * 
 * 封装了设备端服务提供者的所有功能，包括：
 * - 连接管理
 * - 服务注册
 * - 请求处理
 * - 状态监控
 */
class HelloDeviceProvider {
private:
    std::string server_address_;
    std::string device_id_;
    std::map<std::string, ToolFunction> tools_;
    std::atomic<bool> running_;
    DeviceConnectionStatus status_;
    
public:
    /**
     * @brief 构造函数
     * @param server_address 云端服务器地址
     * @param device_id 设备ID
     */
    HelloDeviceProvider(const std::string& server_address, const std::string& device_id)
        : server_address_(server_address), device_id_(device_id), running_(false), 
          status_(DeviceConnectionStatus::DISCONNECTED) {
    }
    
    /**
     * @brief 注册工具函数
     * @param tool_name 工具名称
     * @param tool_func 工具函数
     */
    void RegisterTool(const std::string& tool_name, ToolFunction tool_func) {
        tools_[tool_name] = tool_func;
        std::cout << "[设备端] 注册工具: " << tool_name << std::endl;
    }
    
    /**
     * @brief 启动设备端服务
     */
    bool Start() {
        std::cout << "[设备端] 启动 Hello 服务提供者..." << std::endl;
        std::cout << "[设备端] 云端服务器: " << server_address_ << std::endl;
        std::cout << "[设备端] 设备ID: " << device_id_ << std::endl;
        
        // 模拟连接过程
        OnConnectionStatusChanged(DeviceConnectionStatus::CONNECTING);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 模拟连接成功
        status_ = DeviceConnectionStatus::CONNECTED;
        OnConnectionStatusChanged(status_);
        
        running_ = true;
        
        std::cout << "[设备端] 已注册的工具:" << std::endl;
        for (const auto& tool : tools_) {
            std::cout << "  - " << tool.first << std::endl;
        }
        
        return true;
    }
    
    /**
     * @brief 运行服务循环
     */
    void Run() {
        std::cout << "\n[设备端] Hello 服务已就绪，等待云端调用..." << std::endl;
        std::cout << "[设备端] 按 Ctrl+C 退出\n" << std::endl;
        
        // 向云端注册Hello能力
        if (RegisterToCloud()) {
            std::cout << "[设备端] Hello能力注册成功" << std::endl;
        } else {
            std::cout << "[设备端] Hello能力注册失败" << std::endl;
            return;
        }
        
        int call_count = 0;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // 模拟收到云端调用（实际实现中这会通过RPC框架处理）
            if (call_count % 3 == 0) {  // 每15秒模拟一次调用
                std::cout << "[设备端] 模拟收到云端调用..." << std::endl;
                
                // 模拟调用参数
                SimpleJson mock_params;
                mock_params.set("name", "Alice");
                mock_params.set("message", "Hello from device!");
                
                // 调用工具函数
                if (tools_.find("say_hello") != tools_.end()) {
                    std::string result = tools_["say_hello"](mock_params.toString());
                    std::cout << "[设备端] 工具调用结果已发送到云端" << std::endl;
                }
            }
            
            call_count++;
            
            // 每30秒输出一次状态
            if (call_count % 6 == 0) {
                std::cout << "[设备端] 状态: 运行中，等待云端调用..." << std::endl;
            }
        }
    }
    
    /**
     * @brief 停止服务
     */
    void Stop() {
        std::cout << "\n[设备端] 正在停止服务..." << std::endl;
        running_ = false;
        status_ = DeviceConnectionStatus::DISCONNECTED;
        OnConnectionStatusChanged(status_);
    }

private:
    bool RegisterToCloud() {
        std::cout << "[设备端] 正在向云端注册Hello能力..." << std::endl;
        
        // 模拟向云端发送注册请求
        // 在实际实现中，这里会调用云端的RegisterDevice接口
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "[设备端] 发送注册请求: device_id=" << device_id_ 
                  << ", capabilities=say_hello" << std::endl;
        
        // 模拟云端响应
        std::cout << "[设备端] 收到云端响应: 设备注册成功" << std::endl;
        
        return true;
    }
    
    /**
     * @brief 获取连接状态
     */
    DeviceConnectionStatus GetStatus() const {
        return status_;
    }
};

// 全局变量用于信号处理
std::unique_ptr<HelloDeviceProvider> g_provider;

/**
 * @brief 信号处理函数
 */
void SignalHandler(int signal) {
    std::cout << "\n[设备端] 收到退出信号 (" << signal << ")，正在关闭..." << std::endl;
    if (g_provider) {
        g_provider->Stop();
    }
    exit(0);
}

/**
 * @brief 打印使用说明
 */
void PrintUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " [云端服务器地址]" << std::endl;
    std::cout << "默认云端服务器地址: localhost:50051" << std::endl;
}

/**
 * @brief 主函数 - 设备端 Hello 服务提供者
 * 
 * 实现设备端服务提供者模式：
 * 1. 连接到云端服务器
 * 2. 注册 Hello 服务能力
 * 3. 等待云端调用请求
 * 4. 处理云端的 SayHello 调用
 * 5. 保持连接并响应请求
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，argv[1] 为云端服务器地址（可选，默认为 localhost:50051）
 * @return 程序退出码，0 表示成功
 */
int main(int argc, char** argv) {
    // 解析命令行参数
    std::string server_address = "localhost:50051";
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            PrintUsage(argv[0]);
            return 0;
        }
        server_address = argv[1];
    }
    
    std::cout << "=== 设备端 Hello 服务提供者启动 ===" << std::endl;
    
    try {
        // 创建设备端服务提供者
        g_provider = std::make_unique<HelloDeviceProvider>(server_address, "hello_device_001");
        
        // 注册信号处理
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);
        
        // 注册 Hello 工具
        g_provider->RegisterTool("say_hello", hello_tool_function);
        
        // 启动服务
        if (!g_provider->Start()) {
            std::cerr << "[设备端] 启动失败" << std::endl;
            return 1;
        }
        
        // 运行服务循环
        g_provider->Run();
        
    } catch (const std::exception& e) {
        std::cerr << "[设备端] 异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "[设备端] Hello 服务提供者结束" << std::endl;
    return 0;
}