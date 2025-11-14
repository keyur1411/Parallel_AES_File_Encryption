#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <openssl/evp.h>

bool enc_AES(std::string &text, const unsigned char key[], std::vector<unsigned char> &enc_text)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx){
        std::cerr<<"Unable to create ctx "<<std::endl;
        return false;
    }
    
    int len;
    int ct_len;
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL))
    {
        std::cerr<<"Unable to Intialise ctx "<<std::endl;
        EVP_CIPHER_CTX_free(ctx);
        
        return false;
    }
    enc_text.resize(text.size() + EVP_CIPHER_block_size(EVP_aes_128_ecb()));
    if (1 != EVP_EncryptUpdate(ctx, enc_text.data(), &len, reinterpret_cast<const unsigned char *>(text.c_str()), text.size()))
    {
        std::cerr<<"Unable to encrypt ctx "<<std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    ct_len = len;
    
    if (1 != EVP_EncryptFinal_ex(ctx, enc_text.data() + len, &len))
    {
        std::cerr<<"Unable to encrypt final ctx "<<std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ct_len += len;
    enc_text.resize(ct_len);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool dec_AES(const std::vector<unsigned char> &enc_text, std::string &dtext, const unsigned char *key)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return false;

    int len;
    int pt_len;
    std::vector<unsigned char> buf(enc_text.size());

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (1 != EVP_DecryptUpdate(ctx, buf.data(), &len, enc_text.data(), enc_text.size()))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    pt_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, buf.data() + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    pt_len += len;

    buf.resize(pt_len);
    dtext.assign(buf.begin(), buf.end());

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool encrypt_file(std::string &file_path, const unsigned char key[])
{
    std::ifstream file_stream(file_path, std::ios::in | std::ios::binary);
    if (!file_stream)
    {
        std::cerr << "Error opening file for reading: " << file_path << std::endl;
        return false;
    }

    std::string text((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
    file_stream.close();

    std::vector<unsigned char> enc_text;
    if (!enc_AES(text, key, enc_text))
    {
        std::cerr << "In-memory encryption failed." << std::endl;
        return false;
    }

    std::ofstream outFile(file_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile)
    {
        std::cerr << "Error opening file for writing: " << file_path << std::endl;
        return false;
    }
    outFile.write(reinterpret_cast<const char *>(enc_text.data()), enc_text.size());
    outFile.close();
    return true;
}

bool decrypt_file(std::string &file_path, const unsigned char key[])
{
    std::ifstream file_stream(file_path, std::ios::in | std::ios::binary);
    if (!file_stream)
    {
        std::cerr << "de Error opening file for reading: " << file_path << std::endl;
        return false;
    }

    std::vector<unsigned char> enc_text((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
    file_stream.close();

    std::string text;
    if (!dec_AES(enc_text,text, key))
    {
        std::cerr << " de In-memory decryption failed." << std::endl;
        return false;
    }

    std::ofstream outFile(file_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile)
    {
        std::cerr << "de Error opening file for writing: " << file_path << std::endl;
        return false;
    }
   outFile.write(text.c_str(), text.size());
   outFile.close();
   return true;
}
int main()
{
    std::string file_path;
    std::cin>>file_path;
    const unsigned char key[16] = "123456789012345";
    // encrypt_file(file_path,key);
    decrypt_file(file_path,key);
    // std::string text, dtext;
    // std::cin >> text;
    // std::vector<unsigned char> enc_text;
    // if (enc_AES(text, key, enc_text))
    // {
    //     std::cout << "Ciphertext (hex): ";
    //     for (unsigned char c : enc_text)
    //         printf("%02x", c);
    //     std::cout << "\n";
    // }
    // else
    // {
    //     std::cerr << "Encryption failed!" << std::endl;
    //     return 1;
    // }

    // if (dec_AES(enc_text, dtext, key))
    // {
    //     std::cout << "Decrypttext (hex): " << dtext;
    //     std::cout << "\n";
    // }
    // else
    // {
    //     std::cerr << "Decryption failed!" << std::endl;
    //     return 1;
    // }
    return 0;
}