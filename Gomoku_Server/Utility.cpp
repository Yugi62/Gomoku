#include "Utility.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>

#define SALT_SIZE 16       // 솔트 크기 (16바이트)
#define HASH_SIZE 32       // 해시 크기 (32바이트, SHA-256)
#define ITERATIONS 100000  // 반복 횟수 (보안 강화)

std::string Utility::bytesToHex(const unsigned char* bytes, size_t length)
{
    std::ostringstream oss;

    for (size_t i = 0; i < length; i++)    
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];    

    return oss.str();
}

void Utility::hexToBytes(const std::string& hex, unsigned char* bytes, size_t length)
{
    for (size_t i = 0; i < length; ++i) 
    {
        std::string byteString = hex.substr(i * 2, 2);
        bytes[i] = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
    }
}

std::string Utility::hashPassword(const std::string& password)
{
    unsigned char salt[SALT_SIZE];
    unsigned char hash[HASH_SIZE];

    // 랜덤한 솔트 생성
    if (!RAND_bytes(salt, SALT_SIZE))
    {
        throw std::runtime_error("솔트 생성 실패");
    }

    // PBKDF2 해싱 수행
    if (!PKCS5_PBKDF2_HMAC(
        password.c_str(),       //실제 패스워드
        password.size(),        //실제 패스워드 사이즈
        salt,                   //솔트 저장 공간
        SALT_SIZE,              //솔트 사이즈
        ITERATIONS,             //반복 횟수 (횟수가 많을수록 보안이 올라간다)
        EVP_sha512(),           //사용 알고리즘
        HASH_SIZE,              //해쉬 사이즈 
        hash                    //해쉬 저장 공간
    ))
    {
        throw std::runtime_error("비밀번호 해싱 실패");
    }

    // 해시와 솔트를 16진수 문자열로 변환
    return Utility::bytesToHex(salt, SALT_SIZE) + Utility::bytesToHex(hash, HASH_SIZE);
}

bool Utility::verifyPassword(const std::string password, const std::string storedHash)
{
    //DB에서 해쉬를 못 찾은 경우 즉시 반환
    if (storedHash == "")
        return false;

    unsigned char salt[SALT_SIZE];
    unsigned char hash[HASH_SIZE];
    unsigned char newHash[HASH_SIZE];

    // 저장된 해시에서 솔트와 해시 값 추출
    Utility::hexToBytes(storedHash.substr(0, SALT_SIZE * 2), salt, SALT_SIZE);
    Utility::hexToBytes(storedHash.substr(SALT_SIZE * 2), hash, HASH_SIZE);

    // 입력된 비밀번호를 같은 방식으로 해싱
    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.size(), salt, SALT_SIZE, ITERATIONS, EVP_sha512(), HASH_SIZE, newHash)) {
        throw std::runtime_error("비밀번호 해싱 실패");
    }

    // 기존 해시와 새로운 해시 비교
    return std::memcmp(hash, newHash, HASH_SIZE) == 0;
}

std::string Utility::fillZero(std::string str, int width)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(width) << str;
    str = oss.str();
    return str;
}
