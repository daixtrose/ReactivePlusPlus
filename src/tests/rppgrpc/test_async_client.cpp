#include <snitch/snitch.hpp>

#include <rpp/observers/mock_observer.hpp>
#include <rpp/operators/map.hpp>
#include <rpp/subjects/publish_subject.hpp>

#include <rppgrpc/proto.grpc.pb.h>
#include <rppgrpc/proto.pb.h>
#include <rppgrpc/rppgrpc.hpp>

#include "fakeit.hpp"

struct ClientCallbackReaderWriter : public grpc::ClientCallbackReaderWriter<Input, Output>
{
    using grpc::ClientCallbackReaderWriter<Input, Output>::BindReactor;
};

struct ClientCallbackReader : public grpc::ClientCallbackReader<Output>
{
    using grpc::ClientCallbackReader<Output>::BindReactor;
};

struct ClientCallbackWriter : public grpc::ClientCallbackWriter<Input>
{
    using grpc::ClientCallbackWriter<Input>::BindReactor;
};

TEST_CASE("async client can be casted to rppgrpc")
{
    grpc::ClientContext ctx{};
    Output*             resp{};

    rpp::subjects::publish_subject<Input>  subj{};
    rpp::subjects::publish_subject<Output> out_subj{};


    fakeit::Mock<TestService::StubInterface::async_interface> stub_mock{};
    mock_observer_strategy<uint32_t>                          mock{};
    out_subj.get_observable() | rpp::ops::map([](const Output& out) { return out.value(); }) | rpp::ops::subscribe(mock);


    auto validate_write = [&subj, &mock](auto& stream_mock, auto* reactor) {
        SECTION("write to stream")
        {
            for (auto v : {10, 3, 15, 20})
            {
                Input message{};
                message.set_value(v);

                subj.get_observer().on_next(message);
                fakeit::Verify(OverloadedMethod(stream_mock, Write, void(const Input* req, grpc::WriteOptions)).Matching([&message](const Input* req, grpc::WriteOptions) -> bool { return req->value() == message.value(); })).Once();
                reactor->OnWriteDone(true);
            }
        }
        SECTION("write failed")
        {
            CHECK(mock.get_on_error_count() == 0);
            reactor->OnWriteDone(false);
            CHECK(mock.get_on_error_count() == 1);
        }
    };

    auto validate_read = [&mock, &resp](auto& stream_mock, auto* reactor) {
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
            CHECK(mock.get_on_error_count() == 1);
        }
    };

    SECTION("bidirectional")
    {
        grpc::ClientBidiReactor<::Input, ::Output>* reactor{};

        fakeit::Mock<ClientCallbackReaderWriter> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::Fake(Method(stream_mock, Write));
        fakeit::Fake(Method(stream_mock, WritesDone));
        fakeit::When(Method(stream_mock, Read)).AlwaysDo([&resp](Output* r) { resp = r; });

        fakeit::When(Method(stub_mock, Bidirectional)).AlwaysDo([&ctx, &stream_mock, &reactor](::grpc::ClientContext* context, ::grpc::ClientBidiReactor<::Input, ::Output>* r) {
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
        grpc::ClientReadReactor<Output>* reactor{};

        fakeit::Mock<ClientCallbackReader> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::When(Method(stream_mock, Read)).AlwaysDo([&resp](Output* r) { resp = r; });

        Input message{};

        fakeit::When(Method(stub_mock, ServerSide)).AlwaysDo([&ctx, &stream_mock, &reactor, &message](::grpc::ClientContext* context, const Input* input, ::grpc::ClientReadReactor<::Output>* r) {
            reactor = r;
            CHECK(input == &message);
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
        grpc::ClientWriteReactor<::Input>* reactor{};

        fakeit::Mock<ClientCallbackWriter> stream_mock{};
        fakeit::Fake(Method(stream_mock, StartCall));
        fakeit::Fake(OverloadedMethod(stream_mock, Write, void(const Input* req, grpc::WriteOptions options)));
        fakeit::Fake(Method(stream_mock, WritesDone));

        fakeit::When(Method(stub_mock, ClientSide)).AlwaysDo([&ctx, &stream_mock, &reactor, &resp](::grpc::ClientContext* context, Output* o, ::grpc::ClientWriteReactor<::Input>* r) {
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
