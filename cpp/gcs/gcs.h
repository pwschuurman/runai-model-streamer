#pragma once

#include "common/storage_uri/storage_uri.h"
#include "common/s3_credentials/s3_credentials.h"
#include "common/response/response.h"
#include "common/range/range.h"

namespace runai::llm::streamer::impl::gcs
{

// create client
extern "C" common::ResponseCode runai_create_s3_client(const common::s3::StorageUri_C & uri,  const common::s3::Credentials_C & credentials, void ** client);
// destroy client
extern "C" void runai_remove_s3_client(void * client);
// asynchronous read
extern "C" common::ResponseCode  runai_async_read_s3_client(void * client, unsigned num_ranges, common::Range * ranges, size_t chunk_bytesize, char * buffer);
// wait for asynchronous read response
extern "C" common::ResponseCode  runai_async_response_s3_client(void * client, unsigned * index /* output parameter */);
// stop clients
// Stops the responder of each client, in order to notify callers which sent a request and are waiting for a response
extern "C" void runai_stop_s3_clients();
// release clients
extern "C" void runai_release_s3_clients();

}; //namespace runai::llm::streamer::impl::gcs
