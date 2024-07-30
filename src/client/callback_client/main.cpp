// standard headers
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
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
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        grpc::Status status;
        stub_->async()->SayHello(&context, &request, &reply, [&mu, &cv, &done, &status](grpc::Status s) {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }

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

int main(int argc, char **argv)
{
    const auto &server_address = std::string("localhost:50051");
    TestClient greeter(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::string user("world");
    std::string reply = greeter.SayHello(user);
    std::cout << "Greeter received: " << reply << std::endl;

    return 0;
}
