#include <iostream>
#include <string>
#include <vector>
#include <fstream>      // For file I/O
#include <openssl/evp.h>
#include <openssl/rand.h> // For generating the IV
#include <cstdio>       // For std::remove and std::rename

// We define a standard chunk size for reading/writing. 4096 bytes (4KB) is common.
const size_t CHUNK_SIZE = 4096;
const size_t IV_SIZE = 16; // AES block size is 16 bytes

/**
 * @brief Encrypts a file in chunks using AES-128-CTR.
 * This is secure and does not use padding.
 */
bool encryptFile(const std::string& inFilePath, const std::string& outFilePath, const unsigned char key[]) {
    
    std::ifstream inFile(inFilePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error opening input file: " << inFilePath << std::endl;
        return false;
    }
    std::ofstream outFile(outFilePath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        std::cerr << "Error opening output file: " << outFilePath << std::endl;
        return false;
    }

    // --- 1. Generate a random 16-byte IV ---
    unsigned char iv[IV_SIZE];
    if (1 != RAND_bytes(iv, sizeof(iv))) {
        std::cerr << "Failed to generate random IV." << std::endl;
        return false;
    }

    // --- 2. Write the IV to the start of the output file ---
    // The decryptor will need to read this first.
    outFile.write(reinterpret_cast<const char*>(iv), IV_SIZE);

    // --- 3. Context Setup ---
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    // Initialize CTR mode. We pass the key and our new IV.
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // --- 4. Buffers and Loop ---
    std::vector<unsigned char> readBuf(CHUNK_SIZE);
    std::vector<unsigned char> writeBuf(CHUNK_SIZE); // CTR output is same size as input
    int bytesWritten = 0;

    while (true) {
        inFile.read(reinterpret_cast<char*>(readBuf.data()), CHUNK_SIZE);
        size_t bytesRead = inFile.gcount(); 
        if (bytesRead == 0) break;

        // Encrypt the chunk
        if (1 != EVP_EncryptUpdate(ctx, writeBuf.data(), &bytesWritten, readBuf.data(), bytesRead)) {
            std::cerr << "EVP_EncryptUpdate failed." << std::endl;
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        // Write the encrypted chunk
        if (bytesWritten > 0) {
            outFile.write(reinterpret_cast<const char*>(writeBuf.data()), bytesWritten);
        }
    }

    // --- 5. Finalize (for CTR, this does nothing, but it's good practice) ---
    if (1 != EVP_EncryptFinal_ex(ctx, writeBuf.data(), &bytesWritten)) {
        std::cerr << "EVP_EncryptFinal_ex failed." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    // Write any final bytes (should be 0 for CTR)
    if (bytesWritten > 0) {
        outFile.write(reinterpret_cast<const char*>(writeBuf.data()), bytesWritten);
    }

    EVP_CIPHER_CTX_free(ctx);
    inFile.close();
    outFile.close();
    return true;
}

/**
 * @brief Decrypts a file in chunks using AES-128-CTR.
 */
bool decryptFile(const std::string& inFilePath, const std::string& outFilePath, const unsigned char key[]) {
    
    std::ifstream inFile(inFilePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error opening input file: " << inFilePath << std::endl;
        return false;
    }
    std::ofstream outFile(outFilePath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        std::cerr << "Error opening output file: " << outFilePath << std::endl;
        return false;
    }

    // --- 1. Read the 16-byte IV from the start of the file ---
    unsigned char iv[IV_SIZE];
    inFile.read(reinterpret_cast<char*>(iv), IV_SIZE);
    if (inFile.gcount() != IV_SIZE) {
        std::cerr << "File is too small or corrupt (missing IV)." << std::endl;
        return false;
    }

    // --- 2. Context Setup ---
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    // Initialize with the SAME key and the IV we just read.
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // --- 3. Buffers and Loop ---
    // The rest of the file (after the IV) is the ciphertext.
    std::vector<unsigned char> readBuf(CHUNK_SIZE);
    std::vector<unsigned char> writeBuf(CHUNK_SIZE);
    int bytesWritten = 0;

    while (true) {
        inFile.read(reinterpret_cast<char*>(readBuf.data()), CHUNK_SIZE);
        size_t bytesRead = inFile.gcount();
        if (bytesRead == 0) break;

        if (1 != EVP_DecryptUpdate(ctx, writeBuf.data(), &bytesWritten, readBuf.data(), bytesRead)) {
            std::cerr << "EVP_DecryptUpdate failed." << std::endl;
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        if (bytesWritten > 0) {
            outFile.write(reinterpret_cast<const char*>(writeBuf.data()), bytesWritten);
        }
    }

    // --- 4. Finalize (removes/checks padding - not for CTR) ---
    if (1 != EVP_DecryptFinal_ex(ctx, writeBuf.data(), &bytesWritten)) {
        std::cerr << "Decryption failed. Bad key or corrupt file." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    if (bytesWritten > 0) {
        outFile.write(reinterpret_cast<const char*>(writeBuf.data()), bytesWritten);
    }

    EVP_CIPHER_CTX_free(ctx);
    inFile.close();
    outFile.close();
    return true;
}


int main() {
    std::string testFilename = "my_secure_file.txt";
    std::string tempFilename = "my_secure_file.txt.tmp";
    
    std::ofstream testFile(testFilename);
    testFile << "This is a secret message.\n";
    testFile << "This file is encrypted with CTR mode, which is secure.\n";
    for(int i = 0; i < 500; ++i) {
        testFile << "This is padding line " << i << "\n";
    }
    testFile.close();
    std::cout << "Created '" << testFilename << "' for testing." << std::endl;
    std::cout << "------------------------------------------\n";

    const unsigned char key[16] = "123456789012345"; // 16 bytes
    
    std::cout << "Enter (e) to encrypt or (d) to decrypt the file: ";
    char mode;
    std::cin >> mode;

    if (mode == 'e') {
        std::cout << "Encrypting '" << testFilename << "'..." << std::endl;
        if (encryptFile(testFilename, tempFilename, key)) {
            std::remove(testFilename.c_str()); 
            std::rename(tempFilename.c_str(), testFilename.c_str());
            std::cout << "Encryption successful. '" << testFilename << "' is now encrypted with AES-CTR." << std::endl;
        } else {
            std::cerr << "Encryption failed." << std::endl;
            std::remove(tempFilename.c_str());
        }
    } else if (mode == 'd') {
        std::cout << "Decrypting '" << testFilename << "'..." << std::endl;
        if (decryptFile(testFilename, tempFilename, key)) {
            std::remove(testFilename.c_str());
Services:
            std::rename(tempFilename.c_str(), testFilename.c_str());
            std::cout << "Decryption successful. '" << testFilename << "' is now plaintext." << std::endl;
        } else {
            std::cerr << "Decryption failed." << std::endl;
            std::remove(tempFilename.c_str());
        }
    } else {
        std::cerr << "Invalid mode. Please run again and enter 'e' or 'd'." << std::endl;
    }

    return 0;
}