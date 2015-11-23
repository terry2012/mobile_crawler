#include "common.hpp"

bool startswith(const string& long_str, const string& short_str)
{
        return (short_str.length() <= long_str.length() && equal(short_str.begin(), short_str.end(), long_str.begin()));
}


bool startswith_ignorecase(const string& long_str, const string& short_str)
{
        if (short_str.length() > long_str.length())
                return false;

        auto it_long = long_str.begin();
        auto it_short = short_str.begin();
        while(it_short != short_str.end()) {
                if (tolower(*it_long) != tolower(*it_short))
                        return false;

                it_long += 1;
                it_short += 1;
        }

        return true;
}


bool endswith(const string& long_str, const string& short_str)
{
        return (short_str.length() <= long_str.length()) && equal(long_str.end() - short_str.length(), long_str.end(), short_str.begin());
}


bool endswith_ignorecase(const string& long_str, const string& short_str)
{
        if (short_str.length() > long_str.length())
                return false;

        auto it_long = long_str.end() - short_str.length();
        auto it_short = short_str.begin();
        while(it_short != short_str.end()) {
                if (tolower(*it_long) != tolower(*it_short))
                        return false;

                it_long += 1;
                it_short += 1;
        }

        return true;
}
