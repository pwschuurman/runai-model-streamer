#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring> // For strncpy, memset
#include <thread>   // For std::this_thread::sleep_for (optional for polling)
#include <chrono>   // For std::chrono::milliseconds (optional for polling)

// Your provided headers (ensure they are in include path)
#include "gcs/gcs.h"
#include "gcs/client_mgr/client_mgr.h" // May not be directly used by main, but good for context
#include "common/exception/exception.h"
#include "common/range/range.h"
#include "common/response_code/response_code.h"
#include "common/s3_credentials/s3_credentials.h"
#include "common/storage_uri/storage_uri.h"
#include "utils/logging/logging.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <gcs_uri> <local_file_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " gs://my-bucket/data/my-object.zip /tmp/downloaded_file.zip []" << std::endl;
        return 1;
    }

    std::string gcs_uri_str = argv[1];
    std::string local_file_path = argv[2];
    std::string chunk_csv = argv[3];
    std::stringstream chunk_csv_ss(chunk_csv);
    std::string chunk_dsv;
    while (std::getline(chunk_dsv, token, ',')) {
        std::stringstream chunk_dsv_ss(chunk_dsv);
        
        if (!segment.empty()) { // Handle cases like "0-31," or leading/trailing commas gracefully

            try {
                chunks.push_back(parse_single_chunk(segment));
            } catch (const std::exception& e) {
                // Propagate the error message or handle it as needed
                throw std::runtime_error("Error parsing segment \"" + segment + "\": " + e.what());
            }
        }
    }

    LOG(INFO) << "Attempting to download GCS object: " << gcs_uri_str;
    LOG(INFO) << "Target local file path: " << local_file_path;

    runai::llm::streamer::common::s3::StorageUri uri(gcs_uri_str);
    runai::llm::streamer::common::s3::StorageUri_C uri_c(uri);

    // Credentials - assuming Application Default Credentials (ADC) or environment setup.
    // Pass empty credentials struct. The GCSClient implementation should handle auth.
    runai::llm::streamer::common::s3::Credentials credentials;
    runai::llm::streamer::common::s3::Credentials_C credentials_c(credentials); // Default constructor initializes to empty

    void* client_handle = nullptr;
    runai::llm::streamer::common::ResponseCode rc;

    LOG(INFO) << "Creating GCS client for GCS object: " << gcs_uri_str;
    rc = runai::llm::streamer::impl::gcs::runai_create_s3_client(uri_c, credentials_c, &client_handle);

    if (rc != runai::llm::streamer::common::ResponseCode::Success || client_handle == nullptr) {
        LOG(ERROR) << "Failed to create GCS client. Response Code: " << static_cast<int>(rc);
        return 1;
    }
    LOG(INFO) << "GCS client created successfully.";

    std::ofstream local_file(local_file_path, std::ios::binary | std::ios::out);
    if (!local_file.is_open()) {
        LOG(ERROR) << "Failed to open local file for writing: " << local_file_path;
        runai::llm::streamer::impl::gcs::runai_remove_s3_client(client_handle); // Cleanup
        runai::llm::streamer::impl::gcs::runai_release_s3_clients(); // Optional full cleanup
        return 1;
    }
    LOG(INFO) << "Local file opened successfully: " << local_file_path;

    // Define a buffer for downloads
    const size_t DOWNLOAD_BUFFER_SIZE = 1024 * 1024; // 1 MB
    std::vector<char> download_buffer_vec(DOWNLOAD_BUFFER_SIZE);
    char* buffer_ptr = download_buffer_vec.data();

    runai::llm::streamer::common::Range current_range;
    current_range.start = 0;
    // current_range.length will be set before each read call.

    uint64_t total_bytes_downloaded = 0;
    bool download_finished = false;
    unsigned completed_range_index = 0; // We submit one range at a time

    LOG(INFO) << "Starting download loop...";

    while (!download_finished) {
        current_range.size = DOWNLOAD_BUFFER_SIZE; // Request up to buffer size

        LOG(DEBUG) << "Requesting async read: offset=" << current_range.start << ", length=" << current_range.size;

        // The `chunk_bytesize` parameter is the total size of `buffer_ptr`.
        // `num_ranges` is 1. `ranges` points to `current_range`.
        rc = runai::llm::streamer::impl::gcs::runai_async_read_s3_client(client_handle, 1, &current_range, DOWNLOAD_BUFFER_SIZE, buffer_ptr);

        if (rc != runai::llm::streamer::common::ResponseCode::Success) {
            LOG(ERROR) << "Failed to initiate async read. Response Code: " << static_cast<int>(rc)
                       << " (Offset: " << current_range.start << ")";
            break; // Exit loop on submission failure
        }

        LOG(DEBUG) << "Async read initiated. Waiting for response...";

        // Fetch response (blocking...)
        rc = runai::llm::streamer::impl::gcs::runai_async_response_s3_client(client_handle, &completed_range_index);

        if (rc == runai::llm::streamer::common::ResponseCode::Success) {
            if (completed_range_index != 0) { // We only submitted one range with index 0 implicitly
                LOG(ERROR) << "Async response for unexpected range index: " << completed_range_index;
                break;
            }

            // **CRITICAL ASSUMPTION**: `current_range.length` has been updated by the GCSClient
            // to reflect the *actual* number of bytes read into `buffer_ptr`.
            uint64_t bytes_read_in_chunk = current_range.size;

            LOG(DEBUG) << "Async read successful. Bytes read in this chunk: " << bytes_read_in_chunk;

            if (bytes_read_in_chunk > 0) {
                local_file.write(buffer_ptr, bytes_read_in_chunk);
                if (!local_file) {
                    LOG(ERROR) << "Failed to write " << bytes_read_in_chunk << " bytes to local file: " << local_file_path;
                    download_finished = true; // Mark as finished to exit loop
                    rc = runai::llm::streamer::common::ResponseCode::FileAccessError; // Set error status
                    break;
                }
                total_bytes_downloaded += bytes_read_in_chunk;
                current_range.start += bytes_read_in_chunk;

                // If fewer bytes were read than requested, it usually implies EOF.
                // (Unless DOWNLOAD_BUFFER_SIZE wasn't requested, but here current_range.length was initially set to it)
                if (bytes_read_in_chunk < DOWNLOAD_BUFFER_SIZE) {
                    LOG(INFO) << "Partial read (" << bytes_read_in_chunk << " < " << DOWNLOAD_BUFFER_SIZE << "), assuming EOF.";
                    download_finished = true;
                }
            } else {
                // 0 bytes read with Success code implies EOF.
                LOG(INFO) << "Async read successful with 0 bytes. End of file reached.";
                download_finished = true;
            }
        } else {
            LOG(ERROR) << "Async read operation failed. Response Code: " << static_cast<int>(rc)
                       << " (Offset: " << current_range.start << ")";
            download_finished = true; // Exit loop on error
        }
    } // end while(!download_finished)

    local_file.close();
    LOG(INFO) << "Local file closed.";

    if (rc == runai::llm::streamer::common::ResponseCode::Success || total_bytes_downloaded > 0) {
        // Consider success if any bytes were written and the final state was EOF or Success.
        // If loop exited due to an error *after* some successful writes, total_bytes_downloaded will be > 0
        // but rc might be an error code.
        LOG(INFO) << "Download process finished. Total bytes written to local file: " << total_bytes_downloaded;
        if(rc != runai::llm::streamer::common::ResponseCode::Success) {
             LOG(WARNING) << "Download loop exited with Response Code: " << static_cast<int>(rc) << " after writing some data.";
        }
    } else {
        LOG(ERROR) << "Download failed. Total bytes written: " << total_bytes_downloaded << ". Final RC: " << static_cast<int>(rc);
    }

    LOG(INFO) << "Removing GCS client instance...";
    runai::llm::streamer::impl::gcs::runai_remove_s3_client(client_handle);
    client_handle = nullptr;

    // Optional: Clean up all pooled client resources if the application is shutting down
    // runai::llm::streamer::impl::gcs::runai_release_s3_clients();
    // runai::llm::streamer::impl::gcs::runai_stop_s3_clients(); // Important if clients have background tasks

    LOG(INFO) << "Program finished.";

    // Return 0 on success (or partial success that reached EOF), 1 on error.
    bool overall_success = (rc == runai::llm::streamer::common::ResponseCode::Success);
    return overall_success ? 0 : 1;
}