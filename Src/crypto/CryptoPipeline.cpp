#include "crypto/CryptoPipeline.hpp"
#include <thread>
#include <openssl/rand.h>
#include <openssl/evp.h> // Added for processor_func
#include <iostream>
#include <fstream>      // Added for writer_func
#include <vector>       // Added for processor_func

// A single chunk of data to pass between threads
// This using declaration should ideally be in a shared header
// or at least be consistent with what CryptoPipeline.hpp expects.
using Chunk = std::vector<unsigned char>;

// Constructor: Initialize member variables
CryptoPipeline::CryptoPipeline(size_t chunk_size, size_t queue_size)
    : m_chunk_size(chunk_size),
      m_iv_size(16), // AES block size, based on main.cpp's IV_SIZE
      m_read_queue(queue_size),
      m_write_queue(queue_size),
      m_error_flag(false) 
{}

CryptoPipeline::~CryptoPipeline() {} // Nothing to do here for now

bool CryptoPipeline::encryptFile(const std::string& in_path, const std::string& out_path, const unsigned char key[16]) {
    // **Change:**
    // - 'error_flag' becomes 'm_error_flag'
    // - 'read_queue' becomes 'm_read_queue'
    // - 'write_queue' becomes 'm_write_queue'
    // - 'IV_SIZE' becomes 'm_iv_size'
    // - 'reader_func' becomes 'std::thread reader(&CryptoPipeline::reader_thread_func, this, ...)'
    // - Same for processor_func and writer_func

    // --- Start of Pinned Code (encryptFile_threaded) ---
    m_error_flag = false; // Reset error flag

    unsigned char iv[m_iv_size];
    if (1 != RAND_bytes(iv, sizeof(iv))) { 
        std::cerr << "Failed to generate random IV." << std::endl;
        return false; 
    }

    std::ofstream outFile(out_path, std::ios::binary | std::ios::trunc);
    if (!outFile) { 
        std::cerr << "Error opening output file: " << out_path << std::endl;
        return false; 
    }
    outFile.write(reinterpret_cast<const char*>(iv), m_iv_size);
    outFile.close();

    // Queues are already members, no need to create them.

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx || 1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv)) { 
        std::cerr << "Failed to initialize encryption context." << std::endl;
        if(ctx) EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::thread reader(&CryptoPipeline::reader_thread_func, this, in_path, 0);
    std::thread processor(&CryptoPipeline::processor_thread_func, this, ctx, true);
    std::thread writer(&CryptoPipeline::writer_thread_func, this, out_path, std::ios::app);

    reader.join();
    processor.join();
    writer.join();

    EVP_CIPHER_CTX_free(ctx);
    return !m_error_flag;
    // --- End of Pinned Code ---
}

bool CryptoPipeline::decryptFile(const std::string& in_path, const std::string& out_path, const unsigned char key[16]) {
    // **Apply the same changes as in encryptFile**
    // --- Start of Pinned Code (decryptFile_threaded) ---
    m_error_flag = false;

    unsigned char iv[m_iv_size];
    std::ifstream inFile(in_path, std::ios::binary);
    if (!inFile) { 
        std::cerr << "Error opening input file: " << in_path << std::endl;
        return false; 
    }
    inFile.read(reinterpret_cast<char*>(iv), m_iv_size);
    if (inFile.gcount() != m_iv_size) { 
        std::cerr << "File is too small or corrupt (missing IV)." << std::endl;
        return false; 
    }
    inFile.close();

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx || 1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv)) { 
        std::cerr << "Failed to initialize decryption context." << std::endl;
        if(ctx) EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::thread reader(&CryptoPipeline::reader_thread_func, this, in_path, m_iv_size);
    std::thread processor(&CryptoPipeline::processor_thread_func, this, ctx, false);
    std::thread writer(&CryptoPipeline::writer_thread_func, this, out_path, std::ios::trunc);

    reader.join();
    processor.join();
    writer.join();

    EVP_CIPHER_CTX_free(ctx);
    return !m_error_flag;
    // --- End of Pinned Code ---
}

// --- Private Thread Methods ---

void CryptoPipeline::reader_thread_func(const std::string& path, long start_pos) {
    // **Change:**
    // - 'out_queue' becomes 'm_read_queue'
    // - 'error_flag' becomes 'm_error_flag'
    // - 'CHUNK_SIZE' becomes 'm_chunk_size'
    // --- Start of Pinned Code (reader_func) ---
    try {
        std::ifstream inFile(path, std::ios::binary);
        if (!inFile) { 
            std::cerr << "Reader Error: Failed to open input file." << std::endl;
            m_error_flag = true; 
            return; 
        }

        inFile.seekg(start_pos, std::ios::beg);
        if (!inFile) { 
            std::cerr << "Reader Error: Failed to seek in file." << std::endl;
            m_error_flag = true; 
            return; 
        }

        std::vector<unsigned char> readBuf(m_chunk_size); // Use member
        while (true) {
            if (m_error_flag) break; 
            inFile.read(reinterpret_cast<char*>(readBuf.data()), m_chunk_size);
            size_t bytesRead = inFile.gcount();
            if (bytesRead == 0) break;
            Chunk chunk(readBuf.data(), readBuf.data() + bytesRead);
            m_read_queue.push(std::move(chunk), m_error_flag); // Use member
        }
    } catch (const std::exception& e) {
        std::cerr << "Reader Error: " << e.what() << std::endl;
        m_error_flag = true;
    }
    m_read_queue.finish(); // Use member
    // --- End of Pinned Code ---
}

void CryptoPipeline::processor_thread_func(EVP_CIPHER_CTX *ctx, bool is_encrypting) {
    // **Change:**
    // - 'in_queue' becomes 'm_read_queue'
    // - 'out_queue' becomes 'm_write_queue'
    // - 'error_flag' becomes 'm_error_flag'
    // - 'CHUNK_SIZE' becomes 'm_chunk_size'
    // --- Start of Pinned Code (processor_func) ---
    try {
        // Use m_chunk_size
        std::vector<unsigned char> writeBuf(m_chunk_size + EVP_CIPHER_block_size(EVP_aes_128_ctr()));
        int bytesWritten = 0;
        
        while (true) {
            if (m_error_flag) break; // Use member

            Chunk chunk;
            // Pop will return false when the in_queue is empty and finished
            // Use m_read_queue and m_error_flag
            if (!m_read_queue.pop(chunk, m_error_flag)) {
                break; 
            }

            if (is_encrypting) {
                if (1 != EVP_EncryptUpdate(ctx, writeBuf.data(), &bytesWritten, chunk.data(), chunk.size())) {
                    std::cerr << "Processor Error: EVP_EncryptUpdate failed." << std::endl;
                    m_error_flag = true; // Use member
                    break;
                }
            } else {
                if (1 != EVP_DecryptUpdate(ctx, writeBuf.data(), &bytesWritten, chunk.data(), chunk.size())) {
                    std::cerr << "Processor Error: EVP_DecryptUpdate failed." << std::endl;
                    m_error_flag = true; // Use member
                    break;
                }
            }

            if (bytesWritten > 0) {
                Chunk processed_chunk(writeBuf.data(), writeBuf.data() + bytesWritten);
                // Use m_write_queue and m_error_flag
                m_write_queue.push(std::move(processed_chunk), m_error_flag);
            }
        }

        // --- Finalize (only after the loop is done) ---
        if (!m_error_flag) { // Use member
            if (is_encrypting) {
                if (1 != EVP_EncryptFinal_ex(ctx, writeBuf.data(), &bytesWritten)) {
                    std::cerr << "Processor Error: EVP_EncryptFinal_ex failed." << std::endl;
                    m_error_flag = true; // Use member
                }
            } else {
                 if (1 != EVP_DecryptFinal_ex(ctx, writeBuf.data(), &bytesWritten)) {
                    std::cerr << "Processor Error: EVP_DecryptFinal_ex failed (bad key?)." << std::endl;
                    m_error_flag = true; // Use member
                }
            }

            if (bytesWritten > 0) {
                Chunk final_chunk(writeBuf.data(), writeBuf.data() + bytesWritten);
                // Use m_write_queue and m_error_flag
                m_write_queue.push(std::move(final_chunk), m_error_flag);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Processor Error: " << e.what() << std::endl;
        m_error_flag = true; // Use member
    }
    m_write_queue.finish(); // Use member
    // --- End of Pinned Code ---
}

void CryptoPipeline::writer_thread_func(const std::string& path, std::ios_base::openmode mode) {
    // **Change:**
    // - 'in_queue' becomes 'm_write_queue'
    // - 'error_flag' becomes 'm_error_flag'
    // --- Start of Pinned Code (writer_func) ---
    try {
        std::ofstream outFile(path, std::ios::binary | mode);
        if (!outFile) {
            std::cerr << "Writer Error: Failed to open output file." << std::endl;
            m_error_flag = true; // Use member
            return;
        }

        while (true) {
            if (m_error_flag) break; // Use member

            Chunk chunk;
            // Use m_write_queue and m_error_flag
            if (!m_write_queue.pop(chunk, m_error_flag)) {
                break; // No more data
            }
            
            outFile.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
            if (!outFile) {
                std::cerr << "Writer Error: Failed to write to file (disk full?)." << std::endl;
                m_error_flag = true; // Use member
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Writer Error: " << e.what() << std::endl;
        m_error_flag = true; // Use member
    }
    // --- End of Pinned Code ---
}