#include "test/integration/http_integration.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Fault {
namespace {

class FaultIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
                             public Event::TestUsingSimulatedTime,
                             public HttpIntegrationTest {
public:
  FaultIntegrationTest() : HttpIntegrationTest(Http::CodecClient::Type::HTTP1, GetParam()) {}

  void initializeFilter(const std::string& filter_config) {
    config_helper_.addFilter(filter_config);
    initialize();
  }
};

INSTANTIATE_TEST_SUITE_P(IpVersions, FaultIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// fixfix
TEST_P(FaultIntegrationTest, NoFault) {
  const std::string filter_config =
      R"EOF(
name: envoy.fault
config: {}
)EOF";

  initializeFilter(filter_config);
  codec_client_ = makeHttpConnection(makeClientConnection(lookupPort("http")));
  auto response =
      sendRequestAndWaitForResponse(default_request_headers_, 0, default_response_headers_, 1024);
}

// fixfix
TEST_P(FaultIntegrationTest, ResponseRateLimit) {
  const std::string filter_config =
      R"EOF(
name: envoy.fault
config:
  response_rate_limit:
    fixed_limit:
      limit_kbps: 1
    percentage:
      numerator: 100
)EOF";

  initializeFilter(filter_config);
  codec_client_ = makeHttpConnection(makeClientConnection(lookupPort("http")));
  IntegrationStreamDecoderPtr decoder =
      codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  Buffer::OwnedImpl data(std::string(1152, 'a'));
  upstream_request_->encodeData(data, true);
  decoder->waitForBodyData(1024);

  simTime().sleep(std::chrono::milliseconds(63));
  decoder->waitForBodyData(1088);

  simTime().sleep(std::chrono::milliseconds(63));
  decoder->waitForBodyData(1152);
  decoder->waitForEndStream();
}

} // namespace
} // namespace Fault
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
