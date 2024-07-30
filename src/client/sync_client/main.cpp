// standard headers
#include <iostream>
#include <memory>
#include <string>

// grpc headers
#include <grpcpp/grpcpp.h>
#include <robl/api/service.grpc.pb.h>

using robl::api::HelloRequest;
using robl::api::HelloResponse;
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

    return 0;
}
