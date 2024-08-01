// standard headers
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
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
    void SayHello(void)
    {
        HelloRequest request;
        HelloResponse response;
        grpc::ClientContext context;

        request.set_name("world");

        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        grpc::Status status;

        // The actual RPC.
        stub_->async()->SayHello(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s) {
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
            std::cout << "Greeter received: " << response.message() << std::endl;
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void SubscribeProgress(void)
    {
        class Reader : public grpc::ClientReadReactor<SubscribeProgressResponse>
        {
        public:
            Reader(TestService::Stub *stub, const SubscribeProgressRequest &request)
                : request_(request)
                , done_(false)
            {
                stub->async()->SubscribeProgress(&context_, &request_, this);
                StartRead(&response_);
                StartCall();
            }

            void OnReadDone(bool ok) override
            {
                if (ok)
                {
                    std::cout << "Received: " << response_.progress() << std::endl;
                    StartRead(&response_);
                }
            }

            void OnDone(const grpc::Status &status) override
            {
                std::lock_guard<std::mutex> lock(mu_);
                status_ = status;
                done_ = true;
                cv_.notify_one();
            }

            grpc::Status Await(void)
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [this] { return done_; });
                return std::move(status_);
            }

        private:
            grpc::ClientContext context_;
            const SubscribeProgressRequest request_;
            SubscribeProgressResponse response_;
            std::mutex mu_;
            std::condition_variable cv_;
            grpc::Status status_;
            bool done_;
        };

        SubscribeProgressRequest request;
        Reader reader(stub_.get(), request);
        grpc::Status status = reader.Await();
        if (status.ok())
        {
            std::cout << "Subscription Finished!" << std::endl;
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void Chat()
    {
        class Chatter : public grpc::ClientBidiReactor<ChatRequest, ChatResponse>
        {
        public:
            explicit Chatter(TestService::Stub *stub)
            {
                stub->async()->Chat(&context_, this);
                StartCall();
                NextWrite();
                StartRead(&response_);
            }

            void OnWriteDone(bool /*ok*/) override
            {
                NextWrite();
            }

            void OnReadDone(bool ok) override
            {
                if (ok)
                {
                    std::cout << "Got message " << response_.message() << std::endl;
                    StartRead(&response_);
                }
            }

            void OnDone(const grpc::Status &s) override
            {
                std::lock_guard<std::mutex> lock(mu_);
                status_ = s;
                done_ = true;
                cv_.notify_one();
            }

            grpc::Status Await()
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [this] { return done_; });
                return std::move(status_);
            }

        private:
            void NextWrite()
            {
                ChatRequest request;
                std::string message;
                std::getline(std::cin, *request.mutable_message());

                if (request.message() == "q")
                {
                    std::cout << "You closed the chat" << std::endl;
                    StartWritesDone();
                }
                else
                {
                    StartWrite(&request);
                }
            }
            grpc::ClientContext context_;
            ChatResponse response_;
            std::mutex mu_;
            std::condition_variable cv_;
            grpc::Status status_;
            bool done_ = false;
        };

        Chatter chatter(stub_.get());
        grpc::Status status = chatter.Await();
        if (!status.ok())
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            std::cout << "RouteChat rpc failed." << std::endl;
        }
    }

private:
    std::unique_ptr<TestService::Stub> stub_;
};

int main(int argc, char **argv)
{
    const auto &server_address = std::string("localhost:50051");
    TestClient greeter(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::thread t1(&TestClient::SayHello, &greeter);
    std::thread t2(&TestClient::SubscribeProgress, &greeter);
    std::thread t3(&TestClient::Chat, &greeter);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
