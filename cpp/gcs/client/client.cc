#include <algorithm>
#include <string>
#include <utility>
#include <optional>
#include <vector>
#include <future>
#include <memory>
#include <functional>

#include "google/cloud/storage/client.h"
#include "google/cloud/storage/async/client.h"
#include "google/cloud/storage/async/reader_connection.h"
#include "google/cloud/storage/oauth2/credentials.h"
#include "google/cloud/common_options.h"
#include "google/cloud/grpc_options.h"
#include "google/cloud/future.h"
#include "google/cloud/status_or.h"

#include "gcs/client/client.h"

#include "common/exception/exception.h"
#include "common/response/response.h"

#include "utils/logging/logging.h"
#include "utils/env/env.h"
#include "utils/fd/fd.h"

namespace runai::llm::streamer::impl::gcs
{

GCSClient::GCSClient(const common::s3::StorageUri_C & uri) :
    _stop(false),
    _bucket_name(uri.bucket),
    _path(uri.path)
{
    _client = google::cloud::storage_experimental::AsyncClient(_client_config.options);
}

common::ResponseCode GCSClient::read(size_t offset, size_t bytesize, char * buffer)
{
    google::cloud::storage_experimental::BucketName bucket_name(_bucket_name);

    auto result = _client.ReadObjectRange(bucket_name, _path, offset, bytesize).get();

    if (!result.ok()) {
        const auto & err = result.status();
        LOG(ERROR) << "Failed to download GCS object " << err.code() << ": " << err.message();
        return common::ResponseCode::FileAccessError;
    }

    google::cloud::storage_experimental::ReadPayload payload = result.value();
    size_t chunk_offset = 0;
    for (auto chunk : payload.contents()) {
        size_t bytes_received = chunk_offset + chunk.size();
        if (bytes_received > bytesize) {
            LOG(WARNING) << "GCS read received at least " << bytes_received << " bytes, but only "
                        << bytesize << " were requested for this chunk. This is unexpected."
                        << " Discarding data!" << std::endl;
        } else {
            memcpy(buffer + chunk_offset, chunk.data(), chunk.size());
        }
        chunk_offset += chunk.size();
    }

    std::string range_str = "bytes=" + std::to_string(offset) + "-" + std::to_string(offset + bytesize);
    LOG(SPAM) << "Successfully retrieved '" << _path << "' from '" << _bucket_name << "'."  << range_str;
    return common::ResponseCode::Success;
}


common::Response GCSClient::async_read_response()
{
    if (_responder == nullptr)
    {
        LOG(WARNING) << "Requesting response with uninitialized responder";
        return common::ResponseCode::FinishedError;
    }
    return _responder->pop();
}

common::ResponseCode GCSClient::async_read(unsigned num_ranges, common::Range * ranges, size_t chunk_bytesize, char * buffer)
{
    if (_responder != nullptr && !_responder->finished())
    {
        LOG(ERROR) << "GCS client has not finished the previous async request" << std::endl;
        return common::ResponseCode::BusyError;
    }

    _responder = std::make_shared<common::Responder>(num_ranges);

    // TODO: Add GCS authentication here
    // Authentication is typically handled when creating the AsyncClient,
    // e.g., via options passed to MakeStorageAsyncClientConnection() or by
    // default credentials in the environment (GOOGLE_APPLICATION_CREDENTIALS).
    // No explicit per-request auth is usually needed if client is pre-configured.

    char * buffer_ = buffer;
    common::Range * ranges_ = ranges;
    google::cloud::storage_experimental::BucketName bucket_name(_bucket_name);
    for (unsigned ir = 0; ir < num_ranges && !_stop; ++ir)
    {
        const auto & range_ = *ranges_;

        // split range into chunks
        size_t size = std::max(1UL, range_.size/chunk_bytesize);
        LOG(SPAM) <<"Number of chunks is " << size;

        // each range is divided into chunks (size is the number of chunks)
        // when all the chunks have been read successfuly the response for that range is pushed to the responder
        auto counter = std::make_shared< std::atomic<unsigned> >(size);
        // success flag for the current range is passed to the client
        auto is_success = std::make_shared< std::atomic<bool> >(true);
    
        size_t total_ = range_.size;
        size_t offset_ = range_.start;
        for (unsigned i = 0; i < size && !_stop; ++i)
        {
            size_t bytesize_ = (i == size - 1 ? total_ : chunk_bytesize);

            _client.ReadObjectRange(
                bucket_name,
                _path,
                offset_,
                bytesize_
            ).then([this, // For LOG or other members if needed in future
                    responder = _responder,
                    ir,
                    counter,
                    is_success,
                    bytesize_,
                    buffer_]
                   (auto result_future) {

                auto result = result_future.get();
                if (result.ok()) {
                    google::cloud::storage_experimental::ReadPayload payload = result.value();

                    const auto running = counter->fetch_sub(1);
                    LOG(SPAM) << "Async read succeeded - " << running << " running";

                    size_t chunk_offset = 0;
                    for (auto chunk : payload.contents()) {
                        size_t bytes_received = chunk_offset + chunk.size();
                        if (bytes_received > bytesize_) {
                            LOG(WARNING) << "GCS read received at least " << bytes_received << " bytes, but only "
                                      << bytesize_ << " were requested for this chunk. This is unexpected."
                                      << " Discarding data!" << std::endl;
                        } else {
                            memcpy(buffer_ + chunk_offset, chunk.data(), chunk.size());
                        }
                        chunk_offset += chunk.size();
                    }

                    // send success response only if all the requests have succeeded
                    // note that unsuccessful attempts do not update the counter
                    if (running == 1)
                    {
                        LOG(SPAM) << "All GCS requests for range " << ir << " completed successfully." << std::endl;
                        common::Response r(ir, common::ResponseCode::Success);
                        responder->push(std::move(r));
                    }
                } else {
                    // Note: currently a failure to read any sub range fails the entire read request
                    //       a retry mechanism should be added for failed reads
                    bool previous = is_success->exchange(false);
                    // send error response only once
                    if (previous)
                    {
                        const auto & err = result.status();
                        LOG(ERROR) << "Failed to download GCS object " << err.code() << ": " << err.message();
                        common::Response r(ir, common::ResponseCode::FileAccessError);
                        responder->push(std::move(r));
                    }

                    // CRITICAL: As per S3 logic, do NOT decrement the counter on failure.
                    // This ensures that the success condition (counter reaching 1) is only met
                    // if all constituent operations of the range succeed.
                }
            });

            total_ -= bytesize_;
            offset_ += bytesize_;
            buffer_ += bytesize_;
        }
        ranges_++;
    }

    return _stop ? common::ResponseCode::FinishedError : common::ResponseCode::Success;
}

void GCSClient::stop()
{
    _stop = true;
    if (_responder != nullptr)
    {
        _responder->stop();
    }
}

}; // namespace runai::llm::streamer::impl::gcs
