#include <snitch/snitch.hpp>

#include <rpp/observers/mock_observer.hpp>
#include <rpp/operators/map.hpp>
#include <rpp/subjects/publish_subject.hpp>

#include <rppgrpc/proto.grpc.pb.h>
#include <rppgrpc/proto.pb.h>
#include <rppgrpc/rppgrpc.hpp>

#include "fakeit.hpp"

struct ClientCallbackReaderWriter : public grpc::ClientCallbackReaderWriter<Request, Response>
{
    using grpc::ClientCallbackReaderWriter<Request, Response>::BindReactor;
};

struct ClientCallbackReader : public grpc::ClientCallbackReader<Response>
{
    using grpc::ClientCallbackReader<Response>::BindReactor;
};

struct ClientCallbackWriter : public grpc::ClientCallbackWriter<Request>
{
    using grpc::ClientCallbackWriter<Request>::BindReactor;
};

TEST_CASE("async client can be casted to rppgrpc")
{
    grpc::ClientContext ctx{};
    Response*           resp{};

    rpp::subjects::publish_subject<Request>  subj{};
    rpp::subjects::publish_subject<Response> out_subj{};


    fakeit::Mock<TestService::StubInterface::async_interface> stub_mock{};
    mock_observer_strategy<uint32_t>                          mock{};
    out_subj.get_observable() | rpp::ops::map([](const Response& out) { return out.value(); }) | rpp::ops::subscribe(mock);


    auto validate_write = [&subj, &mock](auto& stream_mock, auto*& reactor) {
        SECTION("write to stream")
        {
            std::vector<Request> requests{};
            for (auto v : {10, 3, 15, 20})
            {
                requests.emplace_back().set_value(v);
            }
            for (const auto& r: requests) {
                subj.get_observer().on_next(r);
                reactor->OnWriteDone(true);
            }
            for (const auto& r: requests) {
                fakeit::Verify(OverloadedMethod(stream_mock, Write, void(const Request* req, grpc::WriteOptions)).Matching([r](const Request* req, grpc::WriteOptions) -> bool { return req->value() == r->value(); })).Once();
            }
        }
        SECTION("write failed")
        {
            CHECK(mock.get_on_error_count() == 0);
            reactor->OnWriteDone(false);
            reactor = nullptr;
            CHECK(mock.get_on_error_count() == 1);
        }
    };

    auto validate_read = [&mock, &resp](auto& stream_mock, auto*& reactor) {
        SECTION("read from stream")
        {
            std::vector<uint32_t> expected_values{};
            for (auto v : {3, 5, 2, 1})
            {
                resp->set_value(v);
                CHECK(mock.get_received_values() == expected_values);

                reactor->OnReadDone(true);
                fakeit::Verify(Method(stream_mock, Read));

                expected_values.push_back(v);
                CHECK(mock.get_received_values() == expected_values);
            }
        }
        SECTION("read failed")
        {
            CHECK(mock.get_on_error_count() == 0);
            reactor->OnReadDone(false);
            reactor = nullptr;
            CHECK(mock.get_on_error_count() == 1);
        }
    };

    SECTION("bidirectional")
    {
        grpc::ClientBidiReactor<::Request, ::Response>* reactor{};

        fakeit::Mock<ClientCallbackReaderWriter> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::Fake(Method(stream_mock, Write));
        fakeit::Fake(Method(stream_mock, WritesDone));
        fakeit::When(Method(stream_mock, Read)).AlwaysDo([&resp](Response* r) { resp = r; });

        fakeit::When(Method(stub_mock, Bidirectional)).AlwaysDo([&ctx, &stream_mock, &reactor](::grpc::ClientContext* context, ::grpc::ClientBidiReactor<::Request, ::Response>* r) {
            reactor = r;
            CHECK(context == &ctx);
            stream_mock.get().BindReactor(reactor);
        });

        rppgrpc::add_reactor(&ctx, stub_mock.get(), &TestService::StubInterface::async_interface::Bidirectional, subj.get_observable(), out_subj.get_observer());
        fakeit::Verify(Method(stub_mock, Bidirectional)).Once();

        fakeit::Verify(Method(stream_mock, StartCall)).Once();
        fakeit::Verify(Method(stream_mock, Read)).Once();

        validate_write(stream_mock, reactor);
        validate_read(stream_mock, reactor);

        if (reactor)
        {
            reactor->OnDone(grpc::Status::OK);
        }

        fakeit::VerifyNoOtherInvocations(stream_mock);
    }
    SECTION("server-side")
    {
        grpc::ClientReadReactor<Response>* reactor{};

        fakeit::Mock<ClientCallbackReader> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::When(Method(stream_mock, Read)).AlwaysDo([&resp](Response* r) { resp = r; });

        Request message{};

        fakeit::When(Method(stub_mock, ServerSide)).AlwaysDo([&ctx, &stream_mock, &reactor, &message](::grpc::ClientContext* context, const Request* request, ::grpc::ClientReadReactor<::Response>* r) {
            reactor = r;
            CHECK(request == &message);
            CHECK(context == &ctx);
            stream_mock.get().BindReactor(reactor);
        });

        rppgrpc::add_reactor(&ctx, stub_mock.get(), &message, &TestService::StubInterface::async_interface::ServerSide, out_subj.get_observer());
        fakeit::Verify(Method(stub_mock, ServerSide)).Once();

        fakeit::Verify(Method(stream_mock, StartCall)).Once();
        fakeit::Verify(Method(stream_mock, Read)).Once();

        validate_read(stream_mock, reactor);

        if (reactor)
        {
            reactor->OnDone(grpc::Status::OK);
        }
        fakeit::VerifyNoOtherInvocations(stream_mock);
    }
    SECTION("client-side")
    {
        grpc::ClientWriteReactor<::Request>* reactor{};

        fakeit::Mock<ClientCallbackWriter> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::Fake(OverloadedMethod(stream_mock, Write, void(const Request* req, grpc::WriteOptions options)));
        fakeit::Fake(Method(stream_mock, WritesDone));

        fakeit::When(Method(stub_mock, ClientSide)).AlwaysDo([&ctx, &stream_mock, &reactor, &resp](::grpc::ClientContext* context, Response* o, ::grpc::ClientWriteReactor<::Request>* r) {
            reactor = r;
            resp    = o;
            CHECK(context == &ctx);
            stream_mock.get().BindReactor(reactor);
        });

        rppgrpc::add_reactor(&ctx, stub_mock.get(), &TestService::StubInterface::async_interface::ClientSide, subj.get_observable(), out_subj.get_observer());
        fakeit::Verify(Method(stub_mock, ClientSide)).Once();

        fakeit::Verify(Method(stream_mock, StartCall)).Once();

        validate_write(stream_mock, reactor);

        SECTION("get response")
        {
            CHECK(resp);
            resp->set_value(30);

            subj.get_observer().on_completed();
            fakeit::Verify(Method(stream_mock, WritesDone)).Once();

            CHECK(mock.get_total_on_next_count() == 0);
            reactor->OnDone(grpc::Status::OK);
            reactor = nullptr;

            CHECK(mock.get_received_values() == std::vector<uint32_t>{30});
        }
        if (reactor)
        {
            reactor->OnDone(grpc::Status::OK);
        }
        fakeit::VerifyNoOtherInvocations(stream_mock);
    }
    fakeit::VerifyNoOtherInvocations(stub_mock);
}
