#include "gcs/gcs.h" // Include the header with the original function names
#include "gcs/client_mgr/client_mgr.h" // Manages GCSClient instances

#include "common/exception/exception.h"
#include "utils/logging/logging.h" // For LOG macro

namespace runai::llm::streamer::impl::gcs
{

// Note: These functions keep their original 's3' names for API compatibility,
// but their implementation uses the GCS C++ SDK via GCSClientMgr and GCSClient.

// Creates a GCS client instance, potentially reusing one from the manager.
// Keeps original name runai_create_s3_client for API compatibility.
common::ResponseCode runai_create_s3_client(const common::s3::StorageUri_C & uri, const common::s3::Credentials_C & credentials, void ** client)
{
    common::ResponseCode ret = common::ResponseCode::Success;
    try
    {
        *client = static_cast<void *>(GCSClientMgr::pop(uri));
    }
    catch(const common::Exception & e)
    {
        ret = e.error();
        *client = nullptr;
    }
    catch(const std::exception & e)
    {
        LOG(ERROR) << "Failed to create GCS client";
        ret = common::ResponseCode::FileAccessError;
        *client = nullptr;
    }
    return ret;
}

// Returns a GCS client instance to the manager for potential reuse.
// Keeps original name runai_remove_s3_client for API compatibility.
void runai_remove_s3_client(void * client)
{
    try
    {
        if (client)
        {
           GCSClientMgr::push(static_cast<GCSClient *>(client));
        }
    }
    catch(const std::exception & e)
    {
        LOG(ERROR) << "Failed to remove GCS client";
    }
}

// Clears all managed GCS client instances.
// Keeps original name runai_release_s3_clients for API compatibility.
void runai_release_s3_clients()
{
    try
    {
        GCSClientMgr::clear();
    }
    catch(const std::exception & e)
    {
        LOG(ERROR) << "Failed to remove all GCS clients";
    }
}

// Stops all managed GCS client instances (e.g., cancels ongoing async operations).
// Keeps original name runai_stop_s3_clients for API compatibility.
void runai_stop_s3_clients()
{
    try
    {
        GCSClientMgr::stop();
    }
    catch(const std::exception & e)
    {
        LOG(ERROR) << "Failed to stop all GCS clients";
    }
}

// Initiates an asynchronous read operation on a specific GCS client.
// Keeps original name runai_async_read_s3_client for API compatibility.
common::ResponseCode runai_async_read_s3_client(void * client, unsigned num_ranges, common::Range * ranges, size_t chunk_bytesize, char * buffer)
{
    try
    {
        if (!client)
        {
            LOG(ERROR) << "Attempt to read with null GCS client";
            return common::ResponseCode::UnknownError;
        }
        auto ptr = static_cast<GCSClient *>(client);
        return ptr->async_read(num_ranges, ranges, chunk_bytesize, buffer);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "Caught exception while sending async request";
    }
    return common::ResponseCode::UnknownError;
}

// Retrieves the result of a completed asynchronous read operation.
// Keeps original name runai_async_response_s3_client for API compatibility.
common::ResponseCode runai_async_response_s3_client(void * client, unsigned * index)
{
    try
    {
        if (!client)
        {
            LOG(ERROR) << "Attempt to get read response with null GCS client";
            return common::ResponseCode::UnknownError;
        }
        auto ptr = static_cast<GCSClient *>(client);
        auto response = ptr->async_read_response();
        *index = response.index;
        return response.ret;
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "Caught exception while sending async request";
    }
    return common::ResponseCode::UnknownError;
}

}; // namespace runai::llm::streamer::impl::gcs
