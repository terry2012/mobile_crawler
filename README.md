# mobile_crawler

This is a simple crawler to collect web pages of mobile version.

#### Install

Four libraries are used:

* boost graph : http://www.boost.org/doc/libs/1_59_0/libs/graph/doc/index.html
* boost regex : http://www.boost.org/doc/libs/1_48_0/libs/regex/doc/html/index.html
* libcurl     : http://curl.haxx.se/libcurl/
* gumbo-parser : https://github.com/google/gumbo-parser

Compilation:

* $ cd \<src_path\>
* $ make


#### Run

Input/Output can be configed in 'config.h'
* Input: PENDING_URL_FILE 
* Ouput: GRAPH_FILE

Run
* $ ./bin/crawler
