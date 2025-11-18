#include <iostream>
#include <string>
#include <chrono>
#include <cstdio> // For std::remove, std::rename
#include <filesystem> // For std::filesystem::file_size

// Project Headers
#include "crypto/CryptoPipeline.hpp"
#include "logging/DatabaseLogger.hpp"

#include <fstream> //for file system

int main() {

    //Checking for Database
    DatabaseLogger logger("newdb.db"); // Create object
    if (!logger.init()) { // Call method
        std::cerr << "Failed to initialize logging database. Exiting." << std::endl;
        return 1;
    }
    std::cout << "Logging database 'file_log.db' initialized." << std::endl;

    CryptoPipeline pipeline; // Create our pipeline object

    //User Input
    std::string testFilename;
    std::cout << "Enter the path to your file (e.g., 'image.jpg'): ";
    std::getline(std::cin, testFilename);
    std::string ftype="";
    std::ifstream checkFile(testFilename, std::ios::binary);
    if (!checkFile) {
        std::cerr << "Error: File not found '" << testFilename << "'" << std::endl;
        logger.log(testFilename, "FILE_NOT_FOUND", "FAILED", 0,0,ftype); // Use object
        return 1;
    }
    checkFile.close();

    long long fsize = std::filesystem::file_size(std::filesystem::path(testFilename)); 
    ftype = std::filesystem::path(testFilename).extension().string();

    std::string tempFilename = testFilename + ".tmp";
    const unsigned char key[16] = "keyurkumarkanja";

    std::cout << "Enter (e) to encrypt or (d) to decrypt the file: ";
    char mode;
    std::cin >> mode;

    //Timer & Operation
    auto start_time = std::chrono::high_resolution_clock::now();
    bool success = false;
    std::string operation;
    std::string status;

    if (mode == 'e') {
        operation = "ENCRYPT";
        std::cout << "Encrypting '" << testFilename << "'..." << std::endl;
        success = pipeline.encryptFile(testFilename, tempFilename, key);

    } else if (mode == 'd') {
        operation = "DECRYPT";
        std::cout << "Decrypting '" << testFilename << "'..." << std::endl;
        success = pipeline.decryptFile(testFilename, tempFilename, key);

    } else {
        std::cerr << "Invalid mode. Please run again and enter 'e' or 'd'." << std::endl;
        logger.log(testFilename, "INVALID_MODE", "FAILED", 0,fsize,ftype); // Use object
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;

    // Finalize & Log
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

    std::cout<<fsize<<" "<<ftype<<std::endl;
    logger.log(testFilename, operation, status, duration_ms.count(),fsize,ftype); // Use object
    std::cout << "Operation logged to 'file_log.db' (" << status << ", " << duration_ms.count() << " ms)." << std::endl;

    return success ? 0 : 1;
}