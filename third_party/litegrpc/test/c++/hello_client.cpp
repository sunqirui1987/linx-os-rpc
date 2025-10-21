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

// Hello 服务的客户端 Stub
class HelloServiceStub : public litegrpc::StubInterface {
public:
    explicit HelloServiceStub(std::shared_ptr<grpc::Channel> channel)
        : StubInterface(channel) {}
    
    grpc::Status SayHello(
        grpc::ClientContext* context,
        const hello_HelloRequest& request,
        hello_HelloResponse* response) {
        
        // 使用 nanopb 序列化请求
        uint8_t request_buffer[hello_HelloRequest_size];
        pb_ostream_t request_stream = pb_ostream_from_buffer(request_buffer, sizeof(request_buffer));
        
        if (!pb_encode(&request_stream, hello_HelloRequest_fields, &request)) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to serialize request");
        }
        
        std::string request_data(reinterpret_cast<char*>(request_buffer), request_stream.bytes_written);
        
        // 调用 RPC 方法
        std::string response_data;
        grpc::Status status = MakeCall(
            "/hello.HelloService/SayHello",
            context,
            request_data,
            &response_data);
        
        if (!status.ok()) {
            return status;
        }
        
        // 使用 nanopb 反序列化响应
        pb_istream_t response_stream = pb_istream_from_buffer(
            reinterpret_cast<const uint8_t*>(response_data.data()), 
            response_data.size());
        
        if (!pb_decode(&response_stream, hello_HelloResponse_fields, response)) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to parse response");
        }
        
        return grpc::Status::OK();
    }
};

// Hello 客户端类
class HelloClient {
private:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<HelloServiceStub> stub_;
    
public:
    HelloClient(const std::string& server_address) {
        // 创建不安全的连接（用于本地测试）
        auto credentials = grpc::InsecureChannelCredentials();
        channel_ = grpc::CreateChannel(server_address, credentials);
        stub_ = std::make_unique<HelloServiceStub>(channel_);
        
        std::cout << "Hello客户端已创建，连接到: " << server_address << std::endl;
    }
    
    bool SayHello(const std::string& name, const std::string& message) {
        // 准备请求
        hello_HelloRequest request = hello_HelloRequest_init_default;
        strncpy(request.name, name.c_str(), sizeof(request.name) - 1);
        strncpy(request.message, message.c_str(), sizeof(request.message) - 1);
        
        hello_HelloResponse response = hello_HelloResponse_init_default;
        grpc::ClientContext context;
        
        // 设置超时时间
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
        context.set_deadline(deadline);
        
        std::cout << "\n发送Hello请求:" << std::endl;
        std::cout << "  Name: " << name << std::endl;
        std::cout << "  Message: " << message << std::endl;
        
        // 调用远程方法
        grpc::Status status = stub_->SayHello(&context, request, &response);
        
        if (status.ok()) {
            std::cout << "\n收到Hello响应:" << std::endl;
            std::cout << "  Reply: " << response.reply << std::endl;
            std::cout << "  Status: " << response.status << std::endl;
            return true;
        } else {
            std::cerr << "\nHello RPC调用失败:" << std::endl;
            std::cerr << "  错误码: " << static_cast<int>(status.error_code()) << std::endl;
            std::cerr << "  错误信息: " << status.error_message() << std::endl;
            return false;
        }
    }
    
    bool TestConnection() {
        std::cout << "测试连接到服务器..." << std::endl;
        
        // 等待连接建立
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
        if (!channel_->WaitForConnected(deadline)) {
            std::cerr << "连接超时" << std::endl;
            return false;
        }
        
        std::cout << "连接成功!" << std::endl;
        return true;
    }
};

void PrintUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " [server_address]" << std::endl;
    std::cout << "  server_address: Go服务器地址 (默认: localhost:50051)" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << std::endl;
    std::cout << "  " << program_name << " localhost:50051" << std::endl;
    std::cout << "  " << program_name << " 192.168.1.100:50051" << std::endl;
}

int main(int argc, char** argv) {
    std::string server_address = "localhost:50051";
    
    // 解析命令行参数
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
        server_address = argv[1];
    }
    
    std::cout << "=== Hello RPC 客户端启动 ===" << std::endl;
    std::cout << "连接到Go服务器: " << server_address << std::endl;
    
    try {
        // 创建客户端
        HelloClient client(server_address);
        
        // 测试连接
        if (!client.TestConnection()) {
            std::cerr << "无法连接到服务器，请确保Go服务器正在运行" << std::endl;
            return -1;
        }
        
        std::cout << "\n=== 开始Hello RPC调用测试 ===" << std::endl;
        
        // 测试调用1
        std::cout << "\n--- 测试调用 1 ---" << std::endl;
        bool success1 = client.SayHello("C++客户端", "你好，Go服务器！");
        
        if (success1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 测试调用2
            std::cout << "\n--- 测试调用 2 ---" << std::endl;
            bool success2 = client.SayHello("LiteGRPC", "这是来自C++的第二条消息");
            
            if (success2) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                // 测试调用3
                std::cout << "\n--- 测试调用 3 ---" << std::endl;
                bool success3 = client.SayHello("测试用户", "测试中文消息处理");
                
                if (success3) {
                    std::cout << "\n=== 所有测试调用成功完成! ===" << std::endl;
                }
            }
        }
        
        // 交互式模式
        std::cout << "\n=== 交互式模式 ===" << std::endl;
        std::cout << "输入消息来测试Hello RPC调用 (输入 'quit' 退出):" << std::endl;
        
        std::string input;
        while (true) {
            std::cout << "\n请输入你的名字 (或 'quit' 退出): ";
            std::getline(std::cin, input);
            
            if (input == "quit" || input == "exit") {
                break;
            }
            
            if (input.empty()) {
                input = "匿名用户";
            }
            
            std::cout << "请输入消息: ";
            std::string message;
            std::getline(std::cin, message);
            
            if (message.empty()) {
                message = "Hello from C++ client!";
            }
            
            client.SayHello(input, message);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "客户端异常: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "\nHello RPC客户端退出" << std::endl;
    return 0;
}