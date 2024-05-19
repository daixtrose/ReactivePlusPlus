#include <rpp/rpp.hpp>

#include <grpc++/create_channel.h>
#include <rppgrpc/rppgrpc.hpp>

#include "protocol.grpc.pb.h"
#include "protocol.pb.h"


int main()
{
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    if (!channel)
    {
        std::cout << "NO CHANNEL" << std::endl;
        return 0;
    }
    auto stub = TestService::NewStub(channel);
    if (!stub)
    {
        std::cout << "NO STUB" << std::endl;
        return 0;
    }

    grpc::ClientContext ctx{};
    auto                d = rpp::composite_disposable_wrapper::make();

    rpp::subjects::publish_subject<std::string> requests{};
    rppgrpc::add_reactor(&ctx,
                         *stub->async(),
                         &TestService::StubInterface::async_interface::Bidirectional,
                         requests.get_observable()
                             | rpp::ops::take_while([](const std::string& v) { return v != "0"; })
                             | rpp::ops::map([](const std::string& v) {
                                   Request i{};
                                   i.set_value(v);
                                   std::cout << "SEND REQUEST" << i.ShortDebugString() << std::endl;
                                   return i;
                               }),
                         rpp::make_lambda_observer(d, [](const Response& v) {
                             std::cout << v.value() << std::endl;
                         }));

    std::cout << "SUBSCRIBED" << std::endl;

    std::string in{};
    while (!d.is_disposed())
    {
        std::getline(std::cin, in);
        requests.get_observer().on_next(in);
        in.clear();
    }

    return 0;
}
