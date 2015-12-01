#include "common.hpp"

bool startswith(const string& long_str, const string& short_str)
{
        return (short_str.length() <= long_str.length() && equal(short_str.begin(), short_str.end(), long_str.begin()));
}


bool startswith_ignorecase(const string& long_str, const string& short_str)
{
        if (short_str.length() > long_str.length())
                return false;

#if 0
        auto it_long = long_str.begin();
        auto it_short = short_str.begin();
        while(it_short != short_str.end()) {
                if (tolower(*it_long) != tolower(*it_short))
                        return false;

                it_long += 1;
                it_short += 1;
        }
#endif

        const char*  long_chr_ptr = long_str.c_str();
        const char*  short_chr_ptr = short_str.c_str();
        if (strncasecmp(long_chr_ptr, short_chr_ptr, short_str.length()) == 0)
                return true;

        return false;
}


bool endswith(const string& long_str, const string& short_str)
{
        return (short_str.length() <= long_str.length()) && equal(long_str.end() - short_str.length(), long_str.end(), short_str.begin());
}


bool endswith_ignorecase(const string& long_str, const string& short_str)
{
        if (short_str.length() > long_str.length())
                return false;

#if 0
        auto it_long = long_str.end() - short_str.length();
        auto it_short = short_str.begin();
        while(it_short != short_str.end()) {
                if (tolower(*it_long) != tolower(*it_short))
                        return false;

                it_long += 1;
                it_short += 1;
        }
#endif
        const char*  long_chr_ptr = long_str.c_str() + long_str.length() - short_str.length();
        const char*  short_chr_ptr = short_str.c_str();
        if (strncasecmp(long_chr_ptr, short_chr_ptr, short_str.length()) == 0)
                return true;

        return false;
}

#if 0
/* Got it from https://gist.github.com/ArtemGr/997887 */
static uint64_t crc64(const string& str)
{
        boost::crc_optimal<64, 0x04C11DB7, 0, 0, false, false> crc;
        crc.process_bytes (str.data(), str.size());
        return crc.checksum();
}

static uint64_t crc64(const string* str)
{
        boost::crc_optimal<64, 0x04C11DB7, 0, 0, false, false> crc;
        crc.process_bytes (str->data(), str->size());
        return crc.checksum();
}
#endif

/* http://www.boost.org/doc/libs/1_37_0/libs/crc/crc_example.cpp */
static uint32_t crc32(const string* str)
{
        boost::crc_32_type result;
        result.process_bytes(str->c_str(), str->length());
        return result.checksum();
}

static uint32_t crc32(const string& str)
{
        boost::crc_32_type result;
        result.process_bytes(str.c_str(), str.length());
        return result.checksum();
}

#if 0
/* Got it from  http://stackoverflow.com/questions/98153/whats-the-best-hashing-algorithm-to-use-on-a-stl-string-when-using-hash-map */
static uint64_t hash64(const string& str, uint64_t seed = 0)
{
        const char*  s = str.c_str();

        uint64_t hash = seed;
        while (*s)
                hash = hash * 101 + *s++;

        return hash;
}

static uint64_t hash64(const string* str, uint64_t seed = 0)
{
        const char*  s = str->c_str();

        uint64_t hash = seed;
        while (*s)
                hash = hash * 101 + *s++;

        return hash;
}
#endif

static uint32_t hash32(const string& str, uint32_t seed = 0)
{
        const char*  s = str.c_str();
        uint32_t hash = seed;
        while (*s)
                hash = hash * 101 + *s++;
        return hash;
}

static uint32_t hash32(const string* str, uint32_t seed = 0)
{
        const char*  s = str->c_str();
        uint32_t hash = seed;
        while (*s)
                hash = hash * 101 + *s++;
        return hash;
}

uint64_t myhash(const string& str)
{
        return ((uint64_t)crc32(str) << 32) + hash32(str);
}

uint64_t myhash(const string* str)
{
        return ((uint64_t)crc32(str) << 32) + hash32(str);
}

/* http://www.cplusplus.com/forum/beginner/73057/ */
char* get_current_time( const char* format)
{
        std::time_t t = std::time(0) ;
        static char cstr[128] ;
        std::strftime( cstr, sizeof(cstr), format, std::localtime(&t) ) ;
        return cstr;
}
