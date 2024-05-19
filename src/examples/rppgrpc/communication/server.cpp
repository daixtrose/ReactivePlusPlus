
#include <grpc++/server_builder.h>
#include <rppgrpc/rppgrpc.hpp>

#include "protocol.grpc.pb.h"
#include "protocol.pb.h"

class Service : public TestService::CallbackService
{
public:
    grpc::ServerBidiReactor<::Request, ::Response>* Bidirectional(::grpc::CallbackServerContext* /*context*/) override
    {
        std::cout << "NEW CONNECTION " << std::endl;
        rpp::subjects::publish_subject<Response> response{};
        rpp::subjects::publish_subject<Request>  request{};
        request.get_observable() | rpp::ops::subscribe([](const Request& s) { std::cout << s.ShortDebugString() << std::endl; });
        return new rppgrpc::details::server_bidi_reactor<Response, decltype(request.get_observer())>(response.get_observable(), request.get_observer());
    }

private:
};

int main()
{
    Service             service{};
    grpc::ServerBuilder builder{};

    std::string server_address("localhost:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}
