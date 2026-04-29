#pragma once

#include <iostream>

class Utility
{
public:
    static std::string bytesToHex(const unsigned char* bytes, size_t length);    
    static void hexToBytes(const std::string& hex, unsigned char* bytes, size_t length);  
    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string password, const std::string storedHash);
    static std::string fillZero(std::string str, int width);
};