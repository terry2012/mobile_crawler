#ifndef __COMMON_H__
#define __COMMON_H__ 1

// c headers
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>
#include <ctime>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

// c++ headers
#include <string>
#include <deque>
#include <exception>
#include <algorithm>
#include <unordered_set>


/* boost headers */
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>


/* curl */
#include <curl/multi.h>

/* pugi */
#include <gumbo.h>

#include "config.hpp"
#include "mylog.hpp"
#include "myexception.hpp"


using namespace std;
using namespace boost;


char* get_current_time( const char* format = "%I:%M:%S");

bool startswith(const string& long_str, const string& short_str);
bool startswith_ignorecase(const string& long_str, const string& short_str);
bool endswith(const string& long_str, const string& short_str);
bool endswith_ignorecase(const string& long_str, const string& short_str);

uint64_t myhash(const string& str);
uint64_t myhash(const string* str);

#endif /* __COMMON_H__ */
