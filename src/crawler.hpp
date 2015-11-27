#ifndef __CRAWLER_H__
#define __CRAWLER_H__ 1

#include "common.hpp"


typedef struct Conn {
public:
        CURL*  easy;
        char  url[URL_MAX_LENGTH];
        char  file_path[FILE_PATH_MAX_LENGTH];
        FILE*  output;
} Conn;


typedef size_t (*write_callback_t)(char* ptr, size_t size, size_t nmemb, void* conn);
typedef int (*callback_t)(Conn*  conn);

typedef bool (*is_crawled_t)(string& url);


template <typename StringArray,
          typename Callback = callback_t,
          typename IsCrawled = is_crawled_t>
class Crawler {
private:
        long            m_file_id;
        CURLM*          m_handler;
        StringArray&    m_pending_urls;
        struct timeval  m_timeout;

        Callback        m_done_callback;
        write_callback_t  m_write_callback;
        IsCrawled       m_is_crawled_callback;

        std::unordered_set<CURL*>  m_eh_array;
        StringArray  m_crawling_urls;

private:
        Crawler(StringArray& urls, Callback done_callback, IsCrawled is_crawled)
                : m_file_id(FILE_START_ID),
                  m_pending_urls(urls),
                  m_done_callback(done_callback),
                  m_write_callback(write_callback),
                  m_is_crawled_callback(is_crawled)
        {
                system("mkdir -p " DEFAULT_OUTPUT_DIR);

                curl_global_init(CURL_GLOBAL_ALL);
                init_multi_handler();
        }

public:
        ~Crawler() {
                if (m_handler != NULL) {
                        curl_multi_cleanup(m_handler);
                        curl_global_cleanup();
                }

                m_handler = NULL;
        }

        void update(StringArray& urls, Callback done_callback) {
                m_pending_urls = urls;
                m_done_callback = done_callback;
        }

        static Crawler<StringArray, Callback>& get_instance(StringArray& urls, Callback done_callback, IsCrawled is_crawled) {
                static Crawler instance(urls, done_callback, is_crawled);
                return instance;
        }

private:
        static size_t  write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
                /* size_t  real_size = (size_t)(size * nmemb); */
                Conn*  conn = (Conn*)userdata;

                return fwrite(ptr, size, nmemb, conn->output);
        }

        int init_conn(CURL* eh, string& url, Conn* conn) {
                conn->easy = eh;
                memset(conn->url, 0, sizeof(conn->url));
                snprintf(conn->url, sizeof(conn->url) - 1, "%s", url.c_str());

                memset(conn->file_path, 0, sizeof(conn->file_path));
                snprintf(conn->file_path, sizeof(conn->file_path) -1, "%s/%ld.html", DEFAULT_OUTPUT_DIR, m_file_id);
                m_file_id += 1;

                conn->output = fopen(conn->file_path, "w+");
                if (NULL == conn->output) {
                        throw MyException(string("@init_easy_handler@ can't open file ") + string(conn->file_path));
                }

                return 0;
        }

        CURL* add_easy_handler(string url) {
                CURL* eh = curl_easy_init();
                if (NULL == eh)
                        throw MyException("@init_easy_handler@ curl_easy_init() failed");

                Conn*  conn = (Conn*)malloc(sizeof(Conn));
                init_conn(eh, url, conn);

                curl_easy_setopt(eh, CURLOPT_URL, url.c_str());
                curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, true);
                curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT);
                curl_easy_setopt(eh, CURLOPT_TIMEOUT, CONNECTION_TIMEOUT);
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, false);
                curl_easy_setopt(eh, CURLOPT_COOKIEFILE, ""); /* enable cookie engine */
                curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, DEFAULT_DNS_SERVERS);

                curl_easy_setopt(eh, CURLOPT_USERAGENT, USERAGENT);
                curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, m_write_callback);
                curl_easy_setopt(eh, CURLOPT_WRITEDATA, conn);
                curl_easy_setopt(eh, CURLOPT_PRIVATE, conn);

                m_eh_array.insert(eh);
                m_crawling_urls.insert(url);

                curl_multi_add_handle(m_handler, eh);

                return eh;
        }

        string extract_domain_from_url(string& url) {
                int  start_pos = 0;
                int  tmp_pos = url.find("://");
                if (tmp_pos != -1)
                        start_pos = tmp_pos + 3;

                boost::regex rgx("^([0-9A-Za-z.-]+)[^0-9A-Za-z.-]?");
                boost::match_results<std::string::iterator> match;
                boost::match_flag_type flags = boost::match_default;
                if (regex_search(url.begin() + start_pos, url.end(), match, rgx, flags))
                        return std::string(match[1].first, match[1].second);

                int  end_pos = url.find("/", start_pos);
                return url.substr(start_pos, end_pos - start_pos);
        }

        int select_feasible_urls_by_domains(StringArray& urls) {
                if (m_pending_urls.size() == 0)
                        return -1;

                int counter = 0;
                auto it = m_pending_urls.begin();

                StringArray domains;
                while (it != m_pending_urls.end()) {
                        if (counter >= CONNECTIONS_MAX_NUMBER) {
                                break;
                        }

                        if (it->length() >= URL_MAX_LENGTH) {
                                throw MyException(string("The url is too long : ") + *it);
                        }

                        string url = *it;
                        if (not m_is_crawled_callback(url)) {
                                string domain = extract_domain_from_url(url);
                                if (domains.count(domain) == 0) {
                                        urls.insert(url);
                                }
                                domains.insert(domain);
                        }

                        counter++;
                        it++;
                }

                return 0;
        }

        int select_feasible_urls(StringArray& urls) {
                if (m_pending_urls.size() == 0)
                        return -1;

                int counter = 0;
                auto it = m_pending_urls.begin();
                while (it != m_pending_urls.end()) {
                        if (counter >= CONNECTIONS_MAX_NUMBER) {
                                break;
                        }

                        if (it->length() >= URL_MAX_LENGTH) {
                                throw MyException(string("The url is too long : ") + *it);
                        }

                        if (urls.count(*it) == 0) {
                                urls.insert(*it);
                        }

                        counter++;
                        it++;
                }

                return 0;
        }

        int select_only_one_feasible_url(string* url) {
                if ((NULL == url) || (m_pending_urls.size() == 0))
                        return -1;

                auto it = m_pending_urls.begin();
                while (it != m_pending_urls.end()) {
                        if (m_crawling_urls.count(*it) == 0) {
                                *url = *it;
                                return 0;
                        }
                        it++;
                }

                return -1;
        }


public:
        int remove_done_url(string& url) {
                m_pending_urls.erase(url);

                return 0;
        }

        int add_new_url(string& url) {
                if (m_pending_urls.count(url))
                        m_pending_urls.insert(url);

                return 0;
        }

private:
        int add_multiple_handlers() {
                StringArray urls;
                urls.clear();

                LOGD("* %ld urls left.", m_pending_urls.size());

                if (select_feasible_urls(urls) != 0) {
                        LOGD("All urls are crawled.");
                        return -1;
                }

                /* for_each(urls.begin(), urls.end(), [](string url) {
                 *                 cout << "* " << url << endl;
                 *         }); */

                auto it = urls.begin();
                while (it != urls.end()) {
                        add_easy_handler(*it);
                        it++;
                }

                return 0;
        }

        int add_only_one_handler() {
                static long counter = 0;

                counter++;
                if (counter > 1000) {
                        LOGD("* %ld urls left.", m_pending_urls.size());
                        counter = 0;
                }


                string url("");
                if (select_only_one_feasible_url(&url) != 0) {
                        LOGD("All urls are crawled.");
                        return -1;
                }
                add_easy_handler(url);

                return 0;
        }

        void init_multi_handler() {
                m_handler = curl_multi_init();
                if (NULL == m_handler)
                        throw "can't init curl";

                curl_multi_setopt(m_handler, CURLMOPT_MAXCONNECTS, CONNECTIONS_MAX_NUMBER);
                curl_multi_setopt(m_handler, CURLMOPT_PIPELINING, 1L);
        }

        int set_timeout() {
                long timeout;
                if (curl_multi_timeout(m_handler, &timeout)) {
                        LOGE("@set_timeout curl_multi_timeout");
                        return -1;
                }
                if(timeout < 0)
                        timeout = MULTI_SELECT_TIMEOUT;
                m_timeout.tv_sec = timeout / 1000;
                m_timeout.tv_usec = (timeout % 1000) * 1000;
                return 0;
        }

public:
        int run() {
                if (NULL == m_handler) {
                        LOGE("m_handler is null.");
                        return -1;
                }

                int  msgs_in_queue = -1;
                int  running = -1;
                int  max_fds = -1;
                fd_set  read_fd;
                fd_set  write_fd;
                fd_set  exec_fd;
                CURLMsg  *msg;

                if (add_multiple_handlers() != 0)
                        return -2;

                while (running) {
                        curl_multi_perform(m_handler, &running);

                        if (running) {
                                FD_ZERO(&read_fd);
                                FD_ZERO(&write_fd);
                                FD_ZERO(&exec_fd);

                                if (curl_multi_fdset(m_handler, &read_fd, &write_fd, &exec_fd, &max_fds)) {
                                        LOGE("curl_multi_fdset");
                                        return -3;
                                }

                                if (set_timeout())
                                        return -4;

                                if (max_fds == -1) {
                                        sleep(MULTI_SELECT_TIMEOUT / 1000);
                                } else {
                                        #if 0
                                        if (0 > select(max_fds + 1, &read_fd, &write_fd, &exec_fd, &m_timeout)) {
                                                LOGE("select(%i,,,,%d)", max_fds + 1, (int)m_timeout.tv_sec);
                                                return -5;
                                        }
                                        #endif
                                        if (0 > select(max_fds + 1, &read_fd, &write_fd, &exec_fd, NULL)) {
                                                LOGE("select(%i)", max_fds + 1);
                                                return -5;
                                        }
                                }
                        }

                        while ((msg = curl_multi_info_read(m_handler, &msgs_in_queue))) {
                                if (CURLMSG_DONE == msg->msg) {
                                        Conn*  conn;
                                        CURL*  eh = msg->easy_handle;
                                        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &conn);

                                        fclose(conn->output);
                                        conn->output = NULL;

                                        string url(conn->url);

                                        m_done_callback(conn);
                                        LOGI("%d - %s", msg->data.result, curl_easy_strerror(msg->data.result));

                                        remove_done_url(url);
                                        free(conn);

                                        curl_multi_remove_handle(m_handler, eh);
                                        curl_easy_cleanup(eh);

                                        m_eh_array.erase(eh);
                                        m_crawling_urls.erase(url);
                                }
                                else {
                                        LOGE("exec_fd: CURLMsg (%d)", msg->msg);
                                }

                                if (add_only_one_handler() == 0) {
                                        running++;
                                        break;
                                }
                        }
                }

                return 0;
        }

        int end() {
                #if 0
                auto it = m_eh_array.begin();
                while (it != m_eh_array.end()) {
                        curl_multi_remove_handle(m_handler, *it);
                        curl_easy_cleanup(*it);

                        it += 1;
                }
                #endif

                return 0;
        }
};

#endif /* __CRAWLER_H__ */
