#include <iostream>
#include <string>
#include <chrono>
#include <cstdio> // For std::remove, std::rename

// Include your new project headers!
#include "crypto/CryptoPipeline.hpp"
#include "logging/DatabaseLogger.hpp"

// Your original file had these includes, main.cpp still needs them
#include <fstream> 

int main() {
    // Copy your original 'main' function code
    // **Change:**
    // - Replace 'init_database()' with object-oriented calls
    // - Replace 'encryptFile_threaded()' with 'pipeline.encryptFile()'

    // --- Start of Pinned Code (main) ---

    // --- 1. Init Objects ---
    DatabaseLogger logger("file_log.db"); // Create object
    if (!logger.init()) { // Call method
        std::cerr << "Failed to initialize logging database. Exiting." << std::endl;
        return 1;
    }
    std::cout << "Logging database 'file_log.db' initialized." << std::endl;

    CryptoPipeline pipeline; // Create our pipeline object

    // --- 2. User Input ---
    std::string testFilename;
    std::cout << "Enter the path to your file (e.g., 'image.jpg'): ";
    std::getline(std::cin, testFilename);

    std::ifstream checkFile(testFilename, std::ios::binary);
    if (!checkFile) {
        std::cerr << "Error: File not found '" << testFilename << "'" << std::endl;
        logger.log(testFilename, "FILE_NOT_FOUND", "FAILED", 0); // Use object
        return 1;
    }
    checkFile.close();

    std::string tempFilename = testFilename + ".tmp";
    const unsigned char key[16] = "keyurkumarkanja";

    std::cout << "Enter (e) to encrypt or (d) to decrypt the file: ";
    char mode;
    std::cin >> mode;

    // --- 3. Timer & Operation ---
    auto start_time = std::chrono::high_resolution_clock::now();
    bool success = false;
    std::string operation;
    std::string status;

    if (mode == 'e') {
        operation = "ENCRYPT";
        std::cout << "Encrypting '" << testFilename << "'..." << std::endl;
        // Use the pipeline object!
        success = pipeline.encryptFile(testFilename, tempFilename, key);

    } else if (mode == 'd') {
        operation = "DECRYPT";
        std::cout << "Decrypting '" << testFilename << "'..." << std::endl;
        // Use the pipeline object!
        success = pipeline.decryptFile(testFilename, tempFilename, key);

    } else {
        std::cerr << "Invalid mode. Please run again and enter 'e' or 'd'." << std::endl;
        logger.log(testFilename, "INVALID_MODE", "FAILED", 0); // Use object
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;

    // --- 4. Finalize & Log ---
    if (success) {
        std::remove(testFilename.c_str()); 
        std::rename(tempFilename.c_str(), testFilename.c_str());
        status = "SUCCESS";
        std::cout << "Operation successful." << std::endl;
    } else {
        std::remove(tempFilename.c_str());
        status = "FAILED";
        std::cerr << "Operation failed." << std::endl;
    }

    logger.log(testFilename, operation, status, duration_ms.count()); // Use object
    std::cout << "Operation logged to 'file_log.db' (" << status << ", " << duration_ms.count() << " ms)." << std::endl;

    return success ? 0 : 1;
    // --- End of Pinned Code ---
}