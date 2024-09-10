#include "HashTable.h"

#include <sstream>
#include <iomanip>

#if USE_CREATE_CRC32_TABLE_FUNCTION
static uint32_t g_crc32_table[CRC32_TABLE_SIZE] = { 0 };

void CreateCrc32Table()
{
    for (uint32_t i = 0; i < CRC32_TABLE_SIZE; i++)
    {
        uint32_t c = i << 24;
        for (int j = 0; j < 8; j++)
        {
            c = (c << 1) ^ ((c & 0x80000000) ? 0x04c11db7 : 0);
        }
        g_crc32_table[i] = c;
    }
}

void PrintCrc32Table()
{
    std::string str = "";

    for (uint32_t i = 0; i < CRC32_TABLE_SIZE; i++)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << g_crc32_table[i];
        std::string s = stream.str();
        transform(s.begin(), s.end(), s.begin(), toupper);

        str += "0x";
        str += s;
        str += ", ";

        if (i % 4 == 3) str += "\n";
        if (i % 64 == 63) str += "\n";
    }

    std::cout << str << std::endl;
}
#endif