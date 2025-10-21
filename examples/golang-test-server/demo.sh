#!/bin/bash

echo "=== LinxOS RPC Golang Demo ==="
echo "这是一个简单的 Golang gRPC 服务器 demo，用于调用 xiaozhi-esp32 设备能力"
echo ""

# 检查是否已编译
if [ ! -f "bin/server" ] || [ ! -f "bin/client" ]; then
    echo "正在编译项目..."
    make build
    if [ $? -ne 0 ]; then
        echo "编译失败！"
        exit 1
    fi
    echo "编译完成！"
    echo ""
fi

echo "启动服务器..."
./bin/server &
SERVER_PID=$!

# 等待服务器启动
sleep 2

echo "运行客户端测试..."
echo ""
./bin/client

echo ""
echo "停止服务器..."
kill $SERVER_PID

echo "Demo 完成！"
echo ""
echo "你可以："
echo "1. 查看 README.md 了解更多使用方法"
echo "2. 修改 proto/device.proto 添加新的 RPC 方法"
echo "3. 在 internal/ 目录下实现具体的设备控制逻辑"
echo "4. 使用 grpcurl 工具进行更多测试"