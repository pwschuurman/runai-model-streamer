#pragma once


#include <atomic>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <future>

#include "gcs/client_configuration/client_configuration.h"

#include "google/cloud/storage/async/client.h"
#include "google/cloud/options.h"

#include "common/storage_uri/storage_uri.h"
#include "common/responder/responder.h"
#include "common/response/response.h"
#include "common/range/range.h"

namespace runai::llm::streamer::impl::gcs
{

struct GCSClient
{
    GCSClient(const common::s3::StorageUri_C & path);

    common::ResponseCode read(size_t offset, size_t bytesize, char * buffer);

    common::ResponseCode async_read(unsigned num_ranges, common::Range * ranges, size_t chunk_bytesize, char * buffer);

    common::Response async_read_response();

    // Stop sending requests to the object store
    // Requests that were already sent cannot be cancelled, since the Aws S3CrtClient does not support aborting requests
    // The S3CrtClient d'tor will wait for response of all teh sent requests, which can take a while
    void stop();

    std::string bucket() const;

    void path(const std::string & path);

 private:
    std::atomic<bool> _stop;
    ClientConfiguration _client_config;
    google::cloud::storage_experimental::AsyncClient _client;
    std::string _bucket_name;
    std::string _path;

    // queue of asynchronous responses
    std::shared_ptr<common::Responder> _responder;
};

}; // namespace runai::llm::streamer::impl::gcs