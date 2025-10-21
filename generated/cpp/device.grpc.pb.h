#pragma once

/**
 * Generated gRPC service definitions for LinxOSDeviceService
 * Compatible with LiteGRPC
 */

#include "device.pb.h"
#include "grpcpp/grpcpp.h"

namespace linxos_device {

class LinxOSDeviceService {
public:
    class Stub {
    public:
        explicit Stub(std::shared_ptr<grpc::Channel> channel);
        
        // Device management
        grpc::Status RegisterDevice(grpc::ClientContext* context,
                                  const RegisterDeviceRequest& request,
                                  RegisterDeviceResponse* response);
        
        grpc::Status Heartbeat(grpc::ClientContext* context,
                             const HeartbeatRequest& request,
                             HeartbeatResponse* response);
        
        // Voice interaction
        grpc::Status VoiceSpeak(grpc::ClientContext* context,
                              const VoiceSpeakRequest& request,
                              VoiceSpeakResponse* response);
        
        grpc::Status VoiceVolume(grpc::ClientContext* context,
                               const VoiceVolumeRequest& request,
                               VoiceVolumeResponse* response);
        
        // Display control
        grpc::Status DisplayExpression(grpc::ClientContext* context,
                                     const DisplayExpressionRequest& request,
                                     DisplayExpressionResponse* response);
        
        grpc::Status DisplayText(grpc::ClientContext* context,
                               const DisplayTextRequest& request,
                               DisplayTextResponse* response);
        
        grpc::Status DisplayBrightness(grpc::ClientContext* context,
                                     const DisplayBrightnessRequest& request,
                                     DisplayBrightnessResponse* response);
        
        // Light control
        grpc::Status LightControl(grpc::ClientContext* context,
                                const LightControlRequest& request,
                                LightControlResponse* response);
        
        grpc::Status LightMode(grpc::ClientContext* context,
                             const LightModeRequest& request,
                             LightModeResponse* response);
        
        // Audio processing
        grpc::Status AudioPlay(grpc::ClientContext* context,
                             const AudioPlayRequest& request,
                             AudioPlayResponse* response);
        
        grpc::Status AudioRecord(grpc::ClientContext* context,
                               const AudioRecordRequest& request,
                               AudioRecordResponse* response);
        
        grpc::Status AudioStop(grpc::ClientContext* context,
                             const AudioStopRequest& request,
                             AudioStopResponse* response);
        
        // System management
        grpc::Status SystemInfo(grpc::ClientContext* context,
                              const SystemInfoRequest& request,
                              SystemInfoResponse* response);
        
        grpc::Status SystemRestart(grpc::ClientContext* context,
                                  const SystemRestartRequest& request,
                                  SystemRestartResponse* response);
        
        grpc::Status SystemWifiReconnect(grpc::ClientContext* context,
                                        const SystemWifiReconnectRequest& request,
                                        SystemWifiReconnectResponse* response);
        
        // Generic tool call
        grpc::Status CallTool(grpc::ClientContext* context,
                            const ToolCallRequest& request,
                            ToolCallResponse* response);
        
    private:
        std::shared_ptr<grpc::Channel> channel_;
    };
    
    static std::unique_ptr<Stub> NewStub(std::shared_ptr<grpc::Channel> channel);
};

} // namespace linxos_device