#pragma once

#include "utils/ThreadSafeQueue.hpp" // We need our queue!
#include <string>
#include <atomic>
#include <openssl/evp.h>
#include <fstream> // For std::ios_base

class CryptoPipeline {
public:
    CryptoPipeline(size_t chunk_size = 4096, size_t queue_size = 20);
    ~CryptoPipeline(); // Good practice

    // These are our main public functions
    bool encryptFile(const std::string& in_path, const std::string& out_path, const unsigned char key[16]);
    bool decryptFile(const std::string& in_path, const std::string& out_path, const unsigned char key[16]);

private:
    // The three thread functions become private methods
    void reader_thread_func(const std::string& path, long start_pos);
    void processor_thread_func(EVP_CIPHER_CTX *ctx, bool is_encrypting);
    void writer_thread_func(const std::string& path, std::ios_base::openmode mode);

    // Member variables
    const size_t m_chunk_size;
    const size_t m_iv_size = 16;

    // The pipeline owns the queues and error flag
    ThreadSafeQueue m_read_queue;
    ThreadSafeQueue m_write_queue;
    std::atomic<bool> m_error_flag;
};