#include "StringUtils.h"

static const std::string g_Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Base64Encode(const uint8_t* source, int32_t size, std::string& encodeString)
{
    int32_t i = 0;
    int32_t j = 0;

    uint8_t charArray3[3];
    uint8_t charArray4[4];

    while (size--)
    {
        charArray3[i++] = *(source++);

        if (i == 3)
        {
            charArray4[0] = ((charArray3[0] & 0xfc) >> 2);
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = ((charArray3[2] & 0x3f));

            for (i = 0; i < 4; ++i)
            {
                encodeString += g_Base64Chars[charArray4[i]];
            }

            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; ++j)
        {
            charArray3[j] = '\0';
        }

        charArray4[0] = ((charArray3[0] & 0xfc) >> 2);
        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
        charArray4[3] = ((charArray3[2] & 0x3f));

        for (j = 0; j < i + 1; ++j)
        {
            encodeString += g_Base64Chars[charArray4[j]];
        }

        while (i++ < 3)
        {
            encodeString += '=';
        }
    }
}

void Base64Decode(const std::string& source, std::string& decodeString)
{
    int32_t size = source.size();
    int32_t i = 0;
    int32_t j = 0;
    int32_t idx = 0;

    uint8_t charArray3[3];
    uint8_t charArray4[4];

    while (size-- && (source[idx] != '=') && IsBase64(source[idx]))
    {
        charArray4[i++] = source[idx]; idx++;

        if (i == 4)
        {
            for (i = 0; i < 4; ++i)
            {
                charArray4[i] = g_Base64Chars.find(charArray4[i]);
            }

            charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

            for (i = 0; i < 3; ++i)
            {
                decodeString += charArray3[i];
            }

            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; ++j)
        {
            charArray4[j] = 0;
        }

        for (j = 0; j < 4; ++j)
        {
            charArray4[j] = g_Base64Chars.find(charArray4[j]);
        }

        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

        for (j = 0; j < i - 1; ++j)
        {
            decodeString += charArray3[j];
        }
    }
}
