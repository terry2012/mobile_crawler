#ifndef __CRAWLER_H__
#define __CRAWLER_H__ 1

#include "common.hpp"


typedef struct Conn {
public:
        CURL*  easy;
        FILE*  output;
        string_hash_t url;
        string_hash_t file_path;
} Conn;


typedef size_t (*write_callback_t)(char* ptr, size_t size, size_t nmemb, void* conn);
typedef int (*callback_t)(Conn*  conn);
typedef bool (*is_crawled_t)(string_hash_t url);


template <typename StringArray,
          typename Callback = callback_t,
          typename IsCrawled = is_crawled_t,
          typename StringHash = string_hash_t>
class Crawler {
private:
        uint64_t        m_file_id;
        CURLM*          m_handler;
        StringArray*    m_pending_urls;
        struct timeval  m_timeout;

        Callback        m_done_callback;
        write_callback_t  m_write_callback;
        IsCrawled       m_is_crawled_callback;

        std::unordered_set<CURL*>  m_eh_array;
        StringArray  m_crawling_urls;

        PublicStringPool<>*  m_psp;

private:
        Crawler(StringArray* urls, Callback done_callback, IsCrawled is_crawled)
                : m_pending_urls(urls),
                  m_done_callback(done_callback),
                  m_write_callback(write_callback),
                  m_is_crawled_callback(is_crawled)
        {
                system("mkdir -p " DEFAULT_OUTPUT_DIR);

                curl_global_init(CURL_GLOBAL_ALL);
                init_multi_handler();

                init_file_id();
                LOGD("init_file_id : %ld", m_file_id);

                m_psp = &(PublicStringPool<>::get_instance());
        }

        int init_file_id() {
                m_file_id = FILE_START_ID;
                auto dir = opendir(DEFAULT_OUTPUT_DIR);
                if(NULL == dir)
                        return -1;

                auto entity = readdir(dir);
                while(entity != NULL) {
                        if (entity->d_type == DT_REG)
                                m_file_id = max(m_file_id, (uint64_t)atol(entity->d_name));
                        entity = readdir(dir);
                }

                if (m_file_id != FILE_START_ID)
                        m_file_id += 100;

                closedir(dir);
                return 0;
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

        static Crawler<StringArray, Callback>& get_instance(StringArray* urls, Callback done_callback, IsCrawled is_crawled) {
                static Crawler instance(urls, done_callback, is_crawled);
                return instance;
        }

private:
        static size_t  write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
                Conn*  conn = (Conn*)userdata;

                return fwrite(ptr, size, nmemb, conn->output);
        }

        int init_conn(CURL* eh, StringHash url, Conn* conn) {
                conn->easy = eh;
                conn->url = url;

                std::stringstream file_path;
                file_path << DEFAULT_OUTPUT_DIR << "/" << m_file_id;
                m_file_id += 1;
                string tmp_file_path =file_path.str();
                conn->file_path = m_psp->add_string(&tmp_file_path);

                conn->output = fopen(tmp_file_path.c_str(), "w+");
                if (NULL == conn->output) {
                        throw MyException("@init_easy_handler@ can't open file " + tmp_file_path);
                }

                return 0;
        }

        CURL* add_easy_handler(StringHash url) {
                CURL* eh = curl_easy_init();
                if (NULL == eh)
                        throw MyException("@init_easy_handler@ curl_easy_init() failed");

                Conn*  conn = (Conn*)malloc(sizeof(Conn));
                init_conn(eh, url, conn);

                curl_easy_setopt(eh, CURLOPT_URL, m_psp->get_string_from_hash(url)->c_str());
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

public:
        string extract_domain_from_url(string* url) {
                if (NULL == url || url->length() == 0)
                        return "";

                int  start_pos = 0;
                int  tmp_pos = url->find("://");
                if (tmp_pos != -1)
                        start_pos = tmp_pos + 3;


#if 0
                boost::regex rgx("^([0-9A-Za-z.-]+)[^0-9A-Za-z.-]?");
                boost::match_results<std::string::iterator> match;
                boost::match_flag_type flags = boost::match_default;
                if (regex_search(url->begin() + start_pos, url->end(), match, rgx, flags))
                        return std::string(match[1].first, match[1].second);
#endif

                int  end_pos = min(url->find("/", start_pos), url->size());
                return url->substr(0, end_pos);
        }
private:
        int select_feasible_urls_by_domains(StringArray* urls) {
                if (m_pending_urls->size() == 0)
                        return -1;

                int counter = 0;
                auto it = m_pending_urls->begin();

                StringArray domains;
                while (it != m_pending_urls->end()) {
                        if (counter >= CONNECTIONS_MAX_NUMBER) {
                                break;
                        }

                        string* url = m_psp->get_string_from_hash(*it);
                        if (it->length() >= URL_MAX_LENGTH) {
                                throw MyException(string("The url is too long : ") + (url != NULL? *url : string("")));
                        }

                        if ((not m_is_crawled_callback(*it)) && (m_crawling_urls.count(*it) == 0)
                            && (urls.count(*it) == 0)) {
                                string domain = extract_domain_from_url(url);
                                StringHash domain_sh = m_psp->add_string(&domain);
                                if (domains.count(domain_sh) == 0) {
                                        urls.insert(*it);
                                        domains.insert(domain_sh);
                                }
                        }

                        counter++;
                        it++;
                }

                return 0;
        }

        int select_feasible_urls(StringArray* urls) {
                if (m_pending_urls->size() == 0)
                        return -1;

                int counter = 0;
                auto it = m_pending_urls->begin();
                while (it != m_pending_urls->end()) {
                        if (counter >= CONNECTIONS_MAX_NUMBER) {
                                break;
                        }

                        const string*  url = m_psp->get_string_from_hash(*it);
                        if (url->length() >= URL_MAX_LENGTH) {
                                throw MyException(string("The url is too long : ") + *url);
                        }

                        if (not m_is_crawled_callback(*it) && m_crawling_urls.count(*it) == 0 && urls->count(*it) == 0)
                                urls->insert(*it);

                        counter++;
                        it++;
                }

                return 0;
        }

        StringHash select_only_one_feasible_url() {
                if (m_pending_urls->size() == 0)
                        return -1;

                auto it = m_pending_urls->begin();
                while (it != m_pending_urls->end()) {
                        if (not m_is_crawled_callback(*it) && m_crawling_urls.count(*it) == 0) {
                                return *it;
                        }
                        it++;
                }

                return 0;
        }


public:
        int remove_done_url(StringHash url) {
                m_pending_urls->erase(url);

                return 0;
        }

        int add_new_url(StringHash url) {
                if (url != 0 && m_pending_urls->count(url) == 0) {
                        m_pending_urls->insert(url);
                        return 0;
                }

                return -1;
        }

private:
        int add_multiple_handlers() {
                StringArray urls;
                urls.clear();

                LOGD("* %ld urls left.", m_pending_urls->size());

                if (select_feasible_urls(&urls) != 0) {
                        LOGD("@add_multiple_handlers@ All urls are crawled.");
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
                        LOGD("* %ld urls left.", m_pending_urls->size());
                        counter = 0;
                }


                StringHash url = select_only_one_feasible_url();
                if (url == 0) {
                        LOGD("@add_only_one_handler@ All urls are crawled.");
                        return -1;
                }
                add_easy_handler(url);

                return 0;
        }

        void init_multi_handler() {
                m_handler = curl_multi_init();
                if (NULL == m_handler)
                        throw MyException("can't init curl");

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

                                        m_done_callback(conn);
                                        LOGI("%d - %s", msg->data.result, curl_easy_strerror(msg->data.result));

                                        remove_done_url(conn->url);
                                        free(conn);

                                        curl_multi_remove_handle(m_handler, eh);
                                        curl_easy_cleanup(eh);

                                        m_eh_array.erase(eh);
                                        m_crawling_urls.erase(conn->url);
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
