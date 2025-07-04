#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <optional>

#include "s3/client_configuration/client_configuration.h"
#include "common/backend_api/response/response.h"

#include "common/path/path.h"
#include "common/storage_uri/storage_uri.h"
#include "common/s3_credentials/s3_credentials.h"
#include "common/s3_wrapper/s3_wrapper.h"
#include "common/shared_queue/shared_queue.h"
#include "common/range/range.h"

namespace runai::llm::streamer::impl::s3
{

struct S3ClientBase
{
    S3ClientBase(const common::s3::Path & path);

    S3ClientBase(const common::s3::Path & path, const common::s3::Credentials_C & credentials);

     // verify that clien's credentials have not change
    bool verify_credentials(const common::s3::Credentials_C & credentials) const;

 protected:
    const std::optional<Aws::String> _key;
    const std::optional<Aws::String> _secret;
    const std::optional<Aws::String> _token;
    const std::optional<Aws::String> _region;
    const std::optional<Aws::String> _endpoint;
    std::unique_ptr<Aws::Auth::AWSCredentials> _client_credentials;

 private:
    bool verify_credentials_member(const std::optional<Aws::String>& client_member, const char* input_member, const char * name) const;
};

struct S3Client : S3ClientBase
{
    S3Client(const common::s3::Path & path);

    S3Client(const common::s3::Path & path, const common::s3::Credentials_C & credentials);

    common::ResponseCode async_read(const common::s3::Path & path, common::backend_api::ObjectRequestId_t request_id,  const common::Range & range, size_t chunk_bytesize, char * buffer);

    common::backend_api::Response async_read_response();

    // Stop sending requests to the object store
    // Requests that were already sent cannot be cancelled, since the Aws S3CrtClient does not support aborting requests
    // The S3CrtClient d'tor will wait for response of all teh sent requests, which can take a while
    void stop();

    using S3ClientBase::verify_credentials;

 private:
    std::atomic<bool> _stop;
    ClientConfiguration _client_config;
    std::unique_ptr<Aws::S3Crt::S3CrtClient> _client;

    // queue of asynchronous responses
    using Responder = common::SharedQueue<common::backend_api::Response>;
    std::shared_ptr<Responder> _responder;
};

}; //namespace runai::llm::streamer::impl::s3
