
#include <grpc++/server_builder.h>

#include "protocol.grpc.pb.h"
#include "protocol.pb.h"


int main()
{
    TestService         service{};
    grpc::ServerBuilder builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}
