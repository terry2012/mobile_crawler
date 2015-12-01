#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__ 1

#include "public_string_pool.hpp"
#include "crawler.hpp"
#include "mygraph.hpp"

template <typename StringHash = string_hash_t,
          typename StringArray = std::unordered_set<StringHash>,
          typename Callback = callback_t >
class Scheduler {
private:
        string  m_pending_urls_file_path;
        string  m_graph_file_path;

        StringArray  m_pending_urls;
        Crawler<StringArray, Callback>*  m_crawler;
        MyGraph<>*  m_graph;
        PublicStringPool<>* m_psp;


private:
        Scheduler(string pending_urls_file_path, string graph_file_path) {
                m_pending_urls_file_path = pending_urls_file_path;
                m_graph_file_path = graph_file_path;
                m_crawler = &(Crawler< StringArray >::get_instance(&m_pending_urls, done_callback, is_crawled));
                m_graph = &(MyGraph<>::get_instance(graph_file_path));
                m_psp = &(PublicStringPool<>::get_instance());

                read_pending_urls_files();
                /* sort(m_pending_urls.begin(), m_pending_urls.end()); */
                /* for_each(m_pending_urls.begin(), m_pending_urls.end(), [](string x) {LOGD("%s", x.c_str());}); */
                LOGD("pending_urls.len = %ld", (long)m_pending_urls.size());
        }

        int read_pending_urls_files() {
                ifstream input_file(m_pending_urls_file_path);
                string line;

                if (not input_file.is_open()) {
                        throw MyException(string("failed to open file ") + m_pending_urls_file_path);
                }

                while (not input_file.eof()) {
                        line.clear();
                        getline(input_file, line);
                        trim(line);

                        if (line.length() == 0 || *(line.begin()) == '#')
                                continue;

                        if (not startswith_ignorecase(line, "http://")
                            && not startswith_ignorecase(line, "https://"))
                                line.insert(0, "http://");

                        StringHash line_sh = m_psp->add_string(&line);
                        if (line_sh != 0)
                                m_pending_urls.insert(line_sh);
                }

                input_file.close();

                return 0;
        }

public:
        ~Scheduler() {
                /* delete m_crawler; */
        }

        static Scheduler& get_instance(string pending_urls_file_path, string graph_file_path) {
                static Scheduler instance(pending_urls_file_path, graph_file_path);
                return instance;
        }

private:
        /* Got it from https://github.com/google/gumbo-parser/blob/master/examples/find_links.cc  */
        static int extract_links(StringHash current_url, GumboNode* node, StringArray* links)
        {
                if (node->type != GUMBO_NODE_ELEMENT) {
                        return -1;
                }

                GumboAttribute* href;
                if (node->v.element.tag == GUMBO_TAG_A &&
                    (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
                        PublicStringPool<>* psp = &(PublicStringPool<>::get_instance());

                        string  value = href->value;
                        /* cout << "* " << value << endl; */
                        string  new_url;
                        string*  to_be_inserted_str = NULL;
                        const string*  url = psp->get_string_from_hash(current_url);
                        trim(value);
                        if (startswith_ignorecase(value, "http://")
                            || startswith_ignorecase(value, "https://")) {
                                to_be_inserted_str = &value;
                        } else if (startswith_ignorecase(value, "javascript:")) {
                                ;
                        } else if (url->find(value) != (size_t)-1){
                                if (startswith_ignorecase(value, "//")) {
                                        static string http_prefix = "http";
                                        static string https_prefix = "https";
                                        if (startswith_ignorecase(*url, http_prefix))
                                                value.insert(0, http_prefix + string(":"));
                                        else
                                                value.insert(0, https_prefix + string(":"));

                                        to_be_inserted_str = &value;
                                } else if (startswith_ignorecase(value, "/")) {
                                        Crawler<StringArray>& c = Crawler<StringArray>::get_instance(NULL, NULL, NULL);
                                        string domain = c.extract_domain_from_url((string*)url);
                                        /* cout << "url: " << *url << " domain: " << domain << endl; */
                                        if (domain.length() > 0) {
                                                new_url = domain + value;
                                                to_be_inserted_str = &new_url;
                                        }
                                } else if (value.length() > 0 && value.find("://", 0, min((size_t)10, value.length())) == (size_t)-1) {
                                        if (value.find("./") == (size_t)-1) {
                                                new_url = *url;
                                                size_t index = new_url.rfind("/");
                                                /* cout << "index: " << index << " " << new_url[index - 1] << new_url[index]  << endl; */
                                                if ((index == (size_t)-1) || index > new_url.size()) {
                                                        new_url.append("/");
                                                        new_url.append(value);
                                                } else {
                                                        if (index > 2 && new_url[index - 1] == '/' && new_url[index - 2] == ':') {
                                                                new_url.append("/");
                                                                new_url.append(value);
                                                        } else {
                                                                new_url.replace(new_url.begin() + index + 1, new_url.end(), value);
                                                        }
                                                }
                                                to_be_inserted_str = &new_url;
                                        }
                                }
                        }

                        if (to_be_inserted_str != NULL) {
                                size_t index = to_be_inserted_str->find("#");
                                if (index != (size_t)-1 && index <= to_be_inserted_str->length()) {
                                        to_be_inserted_str->erase(to_be_inserted_str->begin() + index, to_be_inserted_str->end());
                                }

                                StringHash url_sh = psp->add_string(to_be_inserted_str);
                                if (url_sh != 0) {
                                        /* cout << "inserting " << *to_be_inserted_str << endl; */
                                        links->insert(url_sh);
                                }
                        }
                }

                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                        extract_links(current_url, static_cast<GumboNode*>(children->data[i]), links);
                }

                return 0;
        }

public:
        static int done_callback(Conn *conn) {
                CURL* eh = conn->easy;
                long status_code;
                curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &status_code);

                string empty_str("");
                MyGraph<>& g = MyGraph<>::get_instance(empty_str);
                g.add_vertex(conn->url, conn->file_path, time(NULL), status_code, true, false);


                PublicStringPool<>* psp = &(PublicStringPool<>::get_instance());
                const string*  url = psp->get_string_from_hash(conn->url);
                const string*  file_path = psp->get_string_from_hash(conn->file_path);
                LOGD("==============================");
                LOGD("SC:   %ld", status_code);
                LOGD("URL:  %s", url->c_str());
                LOGD("File: %s", file_path->c_str());


                std::ifstream in(*file_path, std::ios::in | std::ios::binary);
                if (!in) {
                        throw MyException("Can't open " + *file_path);
                }

                std::string contents;
                in.seekg(0, std::ios::end);
                contents.resize(in.tellg());
                in.seekg(0, std::ios::beg);
                in.read(&contents[0], contents.size());
                in.close();

                GumboOutput* html = gumbo_parse(contents.c_str());
                StringArray links;
                extract_links(conn->url, (GumboNode*)html->root, &links);
                gumbo_destroy_output(&kGumboDefaultOptions, html);

                if (links.size() != 0) {
                        Crawler<StringArray>& c = Crawler<StringArray>::get_instance(NULL, NULL, NULL);
                        auto it = links.begin();
                        while (it != links.end()) {
                                if (not g.is_crawled(*it))
                                        c.add_new_url(*it);
                                g.get_vd(*it);
                                g.add_edge(conn->url, *it);
                                it++;
                        }
                }

                return 0;
        }

public:
        static bool is_crawled(StringHash url) {
                MyGraph<>& g = MyGraph<>::get_instance("");
                return g.is_crawled(url);
        }

public:
        int run() {
                try {
                        m_crawler->run();
                } catch (const std::exception& ex) {
                        LOGE("Terminated by an exception: %s", ex.what());
                        this->end();
                }

                return 0;
        }

public:
        int end() {
                ofstream fd(TO_BE_CRAWLED_FILE, ios::out | ios::trunc);

                auto it = m_pending_urls.begin();
                while (it != m_pending_urls.end()) {
                        const string*  url = m_psp->get_string_from_hash(*it);
                        if ((url != NULL) && (url->length() > 4)) {
                                fd << *url;
                                fd << "\n";
                        }

                        it++;
                }

                fd.close();

                m_crawler->end();
                m_graph->end();
                m_psp->end();

                return 0;
        }

};

#endif /* __SCHEDULER_H__ */
