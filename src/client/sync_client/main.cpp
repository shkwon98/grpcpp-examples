// standard headers
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// grpc headers
#include <grpcpp/grpcpp.h>
#include <robl/api/service.grpc.pb.h>

using robl::api::ChatRequest;
using robl::api::ChatResponse;
using robl::api::HelloRequest;
using robl::api::HelloResponse;
using robl::api::SubscribeProgressRequest;
using robl::api::SubscribeProgressResponse;
using robl::api::TestService;

class TestClient
{
public:
    TestClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(TestService::NewStub(channel))
    {
    }

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::string SayHello(const std::string &user)
    {
        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloResponse reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        grpc::ClientContext context;

        // The actual RPC.
        grpc::Status status = stub_->SayHello(&context, request, &reply);

        // Act upon its status.
        if (status.ok())
        {
            return reply.message();
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return "RPC failed";
        }
    }

    void SubscribeProgress(void)
    {
        SubscribeProgressRequest request;
        SubscribeProgressResponse response;

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<SubscribeProgressResponse>> reader(stub_->SubscribeProgress(&context, request));

        while (reader->Read(&response))
        {
            std::cout << "Received: " << response.progress() << std::endl;
        }

        grpc::Status status = reader->Finish();
        if (!status.ok())
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
        else
        {
            std::cout << "Subscription Finished!" << std::endl;
        }
    }

    void Chat(void)
    {
        grpc::ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<ChatRequest, ChatResponse>> stream(stub_->Chat(&context));

        std::thread writer([stream]() {
            ChatRequest request;
            while (std::getline(std::cin, *request.mutable_message()))
            {
                if (request.message() == "q")
                {
                    std::cout << "You closed the chat" << std::endl;
                    break;
                }

                stream->Write(request);
            }
            stream->WritesDone();
        });

        ChatResponse response;
        while (stream->Read(&response))
        {
            std::cout << "Received: " << response.message() << std::endl;
        }

        writer.join();
        grpc::Status status = stream->Finish();
        if (!status.ok())
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<TestService::Stub> stub_;
};

int main(void)
{
    const auto &server_address = std::string("localhost:50051");
    TestClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::string user("world");
    std::string reply = client.SayHello(user);
    std::cout << "Client received: " << reply << std::endl;

    std::thread t1(&TestClient::SubscribeProgress, &client);
    std::thread t2(&TestClient::Chat, &client);

    t1.join();
    t2.join();

    return 0;
}
