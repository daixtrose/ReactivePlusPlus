#include <rpp/rpp.hpp>

#include <grpc++/create_channel.h>
#include <rppgrpc/rppgrpc.hpp>

#include "protocol.grpc.pb.h"
#include "protocol.pb.h"


int main()
{
    auto stub = TestService::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    grpc::ClientContext ctx{};
    rppgrpc::add_reactor(&ctx,
                         *stub->async(),
                         &TestService::StubInterface::async_interface::Bidirectional,
                         rpp::source::from_callable(&std::getchar)
                             | rpp::ops::repeat()
                             | rpp::ops::take_while([](char v) { return v != '0'; })
                             | rpp::ops::subscribe_on(rpp::schedulers::new_thread{})
                             | rpp::ops::map([](char v) {
                                   Input i{};
                                   i.set_value(v);
                                   return i;
                               }),
                         rpp::make_lambda_observer([](const Output& v) {
                             std::cout << v.value() << std::endl;
                         }));
}
