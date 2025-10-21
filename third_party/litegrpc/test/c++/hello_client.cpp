/**
 * @file hello_client.cpp
 * @brief LiteGRPC 设备 Hello 能力调用客户端测试程序
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了一个完整的 gRPC 客户端测试程序，用于调用设备端的 Hello 能力。
 * 
 * 主要功能：
 * - 连接到设备端（Go 实现的设备 Hello 能力服务）
 * - 调用设备端的 Hello 能力
 * - 使用 nanopb 进行消息序列化/反序列化
 * - 执行多种设备 Hello 能力调用测试
 * - 提供交互式测试模式
 * - 演示错误处理和超时控制
 * 
 * 测试场景：
 * - 基本的设备端 SayHello 能力调用
 * - 与设备端的连接状态检测
 * - 设备端中文消息处理能力测试
 * - 批量设备能力调用测试
 * - 用户交互式设备能力测试
 * 
 * 使用方法：
 *   ./hello_client [device_address]
 *   默认连接到设备端 localhost:50052
 */

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <cstring>
#include "hello.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "litegrpc/litegrpc.h"

/**
 * @brief 设备端 Hello 能力调用的客户端存根类
 * 
 * 继承自 litegrpc::StubInterface，实现了调用设备端 Hello 能力的客户端接口。
 * 负责处理设备端 SayHello 能力的调用，包括：
 * - 请求消息的序列化（使用 nanopb）
 * - 设备端 Hello 能力调用
 * - 响应消息的反序列化
 * - 错误处理
 */
class HelloServiceStub : public litegrpc::StubInterface {
public:
    /**
     * @brief 构造函数
     * @param channel gRPC 通道，用于与服务器通信
     */
    explicit HelloServiceStub(std::shared_ptr<grpc::Channel> channel)
        : StubInterface(channel) {}
    
    /**
     * @brief 执行设备端 SayHello 能力调用
     * @param context 客户端上下文，包含超时、元数据等信息
     * @param request Hello 请求消息
     * @param response 用于存储响应的指针
     * @return 设备端能力调用状态
     * 
     * 此方法演示了完整的设备端 Hello 能力调用流程：
     * 1. 使用 nanopb 序列化请求消息
     * 2. 通过底层通道发送设备端能力调用请求
     * 3. 接收并反序列化设备端响应消息
     * 4. 处理可能的错误情况
     */
    grpc::Status SayHello(
        grpc::ClientContext* context,
        const hello_HelloRequest& request,
        hello_HelloResponse* response) {
        
        // 使用 nanopb 序列化请求消息
        uint8_t request_buffer[hello_HelloRequest_size];
        pb_ostream_t request_stream = pb_ostream_from_buffer(request_buffer, sizeof(request_buffer));
        
        if (!pb_encode(&request_stream, hello_HelloRequest_fields, &request)) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to serialize request");
        }
        
        // 将序列化后的数据转换为字符串
        std::string request_data(reinterpret_cast<char*>(request_buffer), request_stream.bytes_written);
        
        // 调用底层 RPC 方法
        std::string response_data;
        grpc::Status status = MakeCall(
            "/hello.HelloService/SayHello",  // RPC 方法路径
            context,                         // 客户端上下文
            request_data,                    // 序列化的请求数据
            &response_data);                 // 用于接收响应数据
        
        if (!status.ok()) {
            return status;
        }
        
        // 使用 nanopb 反序列化响应消息
        pb_istream_t response_stream = pb_istream_from_buffer(
            reinterpret_cast<const uint8_t*>(response_data.data()), 
            response_data.size());
        
        if (!pb_decode(&response_stream, hello_HelloResponse_fields, response)) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to parse response");
        }
        
        return grpc::Status::OK();
    }
};

/**
 * @brief 设备端 Hello 能力调用客户端类
 * 
 * 封装了与设备端交互的所有功能，提供了高级的设备能力调用接口。
 * 主要功能包括：
 * - 管理与设备端的连接
 * - 提供简化的设备端 Hello 能力调用接口
 * - 处理与设备端的连接状态检测
 * - 实现错误处理和日志输出
 * 
 * 使用示例：
 *   HelloClient client("localhost:50052");
 *   client.TestConnection();
 *   client.SayHello("用户名", "消息内容");  // 调用设备端 Hello 能力
 */
class HelloClient {
private:
    std::shared_ptr<grpc::Channel> channel_;    ///< gRPC 通道，用于与设备端通信
    std::unique_ptr<HelloServiceStub> stub_;    ///< 设备端 Hello 能力调用存根，封装设备能力调用
    
public:
    /**
     * @brief 构造函数
     * @param device_address 设备端地址（格式：host:port）
     * 
     * 创建设备能力调用客户端实例并初始化与设备端的连接。
     * 使用不安全的连接方式，适用于本地测试环境。
     * 在生产环境中应使用 SSL 连接。
     */
    HelloClient(const std::string& device_address) {
        // 创建不安全的连接（用于本地测试）
        auto credentials = grpc::InsecureChannelCredentials();
        channel_ = grpc::CreateChannel(device_address, credentials);
        stub_ = std::make_unique<HelloServiceStub>(channel_);
        
        std::cout << "设备端 Hello 能力调用客户端已创建，连接到设备端: " << device_address << std::endl;
    }
    
    /**
     * @brief 调用设备端 Hello 能力
     * @param name 发送者姓名
     * @param message 要发送的消息内容
     * @return 成功返回 true，失败返回 false
     * 
     * 此方法演示了完整的设备端 Hello 能力调用流程：
     * 1. 准备请求消息结构
     * 2. 设置客户端上下文（包括超时）
     * 3. 调用设备端的 SayHello 能力
     * 4. 处理设备端响应并输出结果
     * 5. 处理可能的错误情况
     */
    bool SayHello(const std::string& name, const std::string& message) {
        // 准备请求消息
        hello_HelloRequest request = hello_HelloRequest_init_default;
        strncpy(request.name, name.c_str(), sizeof(request.name) - 1);
        strncpy(request.message, message.c_str(), sizeof(request.message) - 1);
        
        // 准备响应消息和上下文
        hello_HelloResponse response = hello_HelloResponse_init_default;
        grpc::ClientContext context;
        
        // 设置超时时间（10秒）
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
        context.set_deadline(deadline);
        
        // 输出请求信息
        std::cout << "\n调用设备端 Hello 能力:" << std::endl;
        std::cout << "  Name: " << name << std::endl;
        std::cout << "  Message: " << message << std::endl;
        
        // 调用设备端 SayHello 能力
        grpc::Status status = stub_->SayHello(&context, request, &response);
        
        if (status.ok()) {
            // 成功：输出设备端响应信息
            std::cout << "\n收到设备端 Hello 响应:" << std::endl;
            std::cout << "  Reply: " << response.reply << std::endl;
            std::cout << "  Status: " << response.status << std::endl;
            return true;
        } else {
            // 失败：输出错误信息
            std::cerr << "\n设备端 Hello 能力调用失败:" << std::endl;
            std::cerr << "  错误码: " << static_cast<int>(status.error_code()) << std::endl;
            std::cerr << "  错误信息: " << status.error_message() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief 测试与服务器的连接
     * @return 连接成功返回 true，失败返回 false
     * 
     * 此方法用于验证客户端是否能够成功连接到服务器。
     * 在执行实际的 RPC 调用之前，建议先调用此方法检查连接状态。
     * 
     * 连接测试包括：
     * - 等待连接建立（最多5秒）
     * - 验证连接状态
     * - 输出连接结果
     */
    bool TestConnection() {
        std::cout << "测试连接到服务器..." << std::endl;
        
        // 等待连接建立（最多等待5秒）
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
        if (!channel_->WaitForConnected(deadline)) {
            std::cerr << "连接超时" << std::endl;
            return false;
        }
        
        std::cout << "连接成功!" << std::endl;
        return true;
    }
};

/**
 * @brief 打印程序使用说明
 * @param program_name 程序名称（通常是 argv[0]）
 * 
 * 显示程序的命令行参数格式和使用示例，帮助用户正确使用客户端程序。
 */
void PrintUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " [服务器地址]" << std::endl;
    std::cout << "  服务器地址: 可选，默认为 localhost:50052" << std::endl;
    std::cout << "  示例: " << program_name << " localhost:50051" << std::endl;
}

/**
 * @brief 主函数 - Hello 客户端测试程序入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码（0表示成功，非0表示失败）
 * 
 * 主要功能：
 * 1. 解析命令行参数，确定服务器地址
 * 2. 创建 HelloClient 实例
 * 3. 测试与服务器的连接
 * 4. 执行预定义的测试用例
 * 5. 进入交互式模式，允许用户手动输入测试数据
 * 
 * 测试场景包括：
 * - 基本的英文消息测试
 * - 中文消息测试（验证 UTF-8 编码支持）
 * - 长消息测试（验证大数据传输）
 * - 交互式测试（用户自定义输入）
 */
int main(int argc, char** argv) {
    std::string server_address = "localhost:50052";  // 默认服务器地址
    
    // 解析命令行参数
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            PrintUsage(argv[0]);
            return 0;
        }
        server_address = argv[1];
    }
    
    std::cout << "=== LiteGRPC Hello 客户端测试 ===" << std::endl;
    std::cout << "目标服务器: " << server_address << std::endl;
    
    // 创建客户端实例
    HelloClient client(server_address);
    
    // 测试连接状态
    if (!client.TestConnection()) {
        std::cerr << "无法连接到服务器，请确保服务器正在运行" << std::endl;
        return 1;
    }
    
    // 执行预定义的测试用例
    std::cout << "\n=== 执行测试调用 ===" << std::endl;
    
    // 测试1：基本英文消息
    client.SayHello("Alice", "Hello from LiteGRPC!");
    
    // 测试2：中文消息（验证 UTF-8 编码支持）
    client.SayHello("小明", "你好，这是来自LiteGRPC的消息！");
    
    // 测试3：长消息（验证大数据传输能力）
    client.SayHello("Bob", "This is a longer message to test the gRPC communication with more content.");
    
    // 进入交互式模式
    std::cout << "\n=== 交互式模式 ===" << std::endl;
    std::cout << "输入 'quit' 或 'exit' 退出" << std::endl;
    
    std::string name, message;
    while (true) {
        std::cout << "\n请输入姓名 (或 'quit' 退出): ";
        std::getline(std::cin, name);
        
        // 检查退出条件
        if (name == "quit" || name == "exit") {
            break;
        }
        
        // 跳过空输入
        if (name.empty()) {
            continue;
        }
        
        // 获取消息内容
        std::cout << "请输入消息: ";
        std::getline(std::cin, message);
        
        // 如果消息为空，使用默认消息
        if (message.empty()) {
            message = "Hello from interactive mode!";
        }
        
        // 发送用户自定义的消息
        client.SayHello(name, message);
    }
    
    std::cout << "\n客户端测试完成！" << std::endl;
    return 0;
}