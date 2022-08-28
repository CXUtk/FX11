#pragma once
#include <cstdlib>
#include <cstdio>
#include <string>

inline bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool IsSpace(char c)
{
    return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

inline bool IsAlphaNumeric(char c)
{
    return IsDigit(c) || IsAlpha(c);
}

inline bool IsNewline(char c)
{
    return c == '\r' || c == '\n';
}

static inline bool IsBase64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

void Base64Encode(const uint8_t* source, int32_t size, std::string& encodeString);

void Base64Decode(const std::string& source, std::string& decodeString);
