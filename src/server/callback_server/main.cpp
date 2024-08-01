// standard headers
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// grpc headers
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <robl/api/service.grpc.pb.h>

using robl::api::ChatRequest;
using robl::api::ChatResponse;
using robl::api::HelloRequest;
using robl::api::HelloResponse;
using robl::api::SubscribeProgressRequest;
using robl::api::SubscribeProgressResponse;
using robl::api::TestService;

// Logic and data behind the server's behavior.
class TestServiceImpl final : public TestService::CallbackService
{
    grpc::ServerUnaryReactor *SayHello(grpc::CallbackServerContext *context, const HelloRequest *request,
                                       HelloResponse *reply) override
    {
        auto *reactor = context->DefaultReactor();

        reply->set_message("Hello " + request->name());
        reactor->Finish(grpc::Status::OK);

        return reactor;
    }

    grpc::ServerWriteReactor<SubscribeProgressResponse> *SubscribeProgress(grpc::CallbackServerContext *context,
                                                                           const SubscribeProgressRequest *request) override
    {
        class SubscribeProgressReactor final : public grpc::ServerWriteReactor<SubscribeProgressResponse>
        {
        public:
            SubscribeProgressReactor(const grpc::CallbackServerContext *context, const SubscribeProgressRequest *request)
                : context_(context)
                , request_(request)
                , total_steps_(19)
                , current_step_(0)
            {
                NextWrite();
            }
            ~SubscribeProgressReactor()
            {
                std::cout << "SubscribeProgressReactor deleted" << std::endl;
            }

            void OnDone(void) override
            {
                delete this;
            }

            void OnWriteDone(bool ok) override
            {
                NextWrite();
            }

        private:
            void NextWrite(void)
            {
                if (context_->IsCancelled())
                {
                    Finish(grpc::Status::CANCELLED);
                    return;
                }

                if (current_step_ <= total_steps_)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    response_.set_progress(100.0 * current_step_++ / total_steps_);
                    StartWrite(&response_);
                    return;
                }

                Finish(grpc::Status::OK);
            }

            const grpc::CallbackServerContext *context_;
            const SubscribeProgressRequest *request_;
            SubscribeProgressResponse response_;

            int total_steps_;
            int current_step_;
        };

        return new SubscribeProgressReactor(context, request);
    }

    grpc::ServerBidiReactor<ChatRequest, ChatResponse> *Chat(grpc::CallbackServerContext *context) override
    {
        class ChatReactor final : public grpc::ServerBidiReactor<ChatRequest, ChatResponse>
        {
        public:
            ChatReactor(const grpc::CallbackServerContext *context)
                : context_(context)
                , done_(false)
            {
                StartRead(&request_);
                sender_ = std::thread(&ChatReactor::SendEverySecond, this);
            }
            ~ChatReactor()
            {
                std::cout << "ChatReactor deleted" << std::endl;
            }

            void OnDone() override
            {
                std::cout << "ChatReactor::OnDone" << std::endl;
                done_ = true;
                sender_.join();
                delete this;
            }

            void OnReadDone(bool ok) override
            {
                if (ok)
                {
                    response_.set_message("You said: " + request_.message());
                    StartWrite(&response_);
                    StartRead(&request_);
                }
                else
                {
                    Finish(grpc::Status::OK);
                }
            }

            void OnWriteDone(bool ok) override
            {
            }

            void SendEverySecond()
            {
                while (!done_)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    time_response_.set_message("Server: " + std::to_string(std::time(nullptr)));
                    if (!done_) // Check for cancellation before writing
                    {
                        StartWrite(&time_response_);
                    }
                }
            }

        private:
            const grpc::CallbackServerContext *context_;
            ChatRequest request_;
            ChatResponse response_;
            ChatResponse time_response_;
            std::thread sender_;
            std::atomic_bool done_;
        };

        return new ChatReactor(context);
    }
};

void RunServer(void)
{
    const auto &server_address = std::string("localhost:50051");
    TestServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(void)
{
    RunServer();
    return 0;
}
