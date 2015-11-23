#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__ 1

#include "crawler.hpp"
#include "mygraph.hpp"

template <typename StringArray = deque<string>, typename Callback = callback_t>
class Scheduler {
private:
        string  m_pending_urls_file_path;
        string  m_graph_file_path;

        StringArray  m_pending_urls;
        Crawler<StringArray, Callback>*  m_crawler;
        MyGraph<>*  m_graph;


private:
        Scheduler(string pending_urls_file_path, string graph_file_path) {
                m_pending_urls_file_path = pending_urls_file_path;
                m_graph_file_path = graph_file_path;
                m_crawler = &(Crawler< StringArray, Callback >::get_instance(m_pending_urls, done_callback));
                m_graph = &(MyGraph<>::get_instance(graph_file_path));

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

                        if (find(m_pending_urls.begin(), m_pending_urls.end(), line) == m_pending_urls.end())
                                m_pending_urls.push_back(line);
                }

                /* TODO: initialize graph */

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
        static int extract_links(char* current_url, GumboNode* node, StringArray& links)
        {
                if (node->type != GUMBO_NODE_ELEMENT) {
                        return -1;
                }

                GumboAttribute* href;
                if (node->v.element.tag == GUMBO_TAG_A &&
                    (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
                        string value(href->value);
                        if (startswith_ignorecase(value, "http://")
                            || startswith_ignorecase(value, "https://")) {
                                links.push_back(value);
                        } else if (startswith_ignorecase(value, "javascript:")) {
                                ;
                        } else if (value.find("://", 0, min((size_t)10, value.length())) == std::string::npos) {
                                string url(current_url);
                                size_t index = url.rfind("/");
                                if (*(value.begin()) != '/')
                                        value.insert(0, "/");
                                if (url[index - 1] == '/') {
                                        url = url.append(value);
                                } else {
                                        url.replace(url.begin() + index, url.end(), value);
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

                string url(conn->url);
                string file_path(conn->file_path);
                string empty_str("");
                MyGraph<>& g = MyGraph<>::get_instance(empty_str);
                g.add_vertex(url, file_path, time(NULL), status_code, true, false);


                LOGD("==============================");
                LOGD("SC:   %ld", status_code);
                LOGD("URL:  %s", conn->url);
                LOGD("File: %s", conn->file_path);


                std::ifstream in(conn->file_path, std::ios::in | std::ios::binary);
                if (!in) {
                        throw MyException(string("Can't open ") + string(conn->file_path));
                }

                std::string contents;
                in.seekg(0, std::ios::end);
                contents.resize(in.tellg());
                in.seekg(0, std::ios::beg);
                in.read(&contents[0], contents.size());
                in.close();

                GumboOutput* html = gumbo_parse(contents.c_str());
                StringArray links;
                extract_links(conn->url, (GumboNode*)html->root, links);
                gumbo_destroy_output(&kGumboDefaultOptions, html);

                if (links.size() != 0) {
                        StringArray sa;
                        Crawler<StringArray, Callback>& c = Crawler<StringArray, Callback>::get_instance(sa, NULL);
                        for_each(links.begin(), links.end(), [&url, &c, &g](string& x) {
                                        if (not g.is_crawled(x))
                                                c.add_new_url(x);
                                        g.get_vd(x);
                                        g.add_edge(url, x);
                                });
                }

                return 0;
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
                ofstream fd(m_pending_urls_file_path, ios::out | ios::trunc);

                auto it = m_pending_urls.begin();
                while (it != m_pending_urls.end()) {
                        if (it->length() > 4) {
                                fd << *it;
                                fd << "\n";
                        }

                        it += 1;
                }

                fd.close();

                m_crawler->end();
                m_graph->end();

                return 0;
        }

};

#endif /* __SCHEDULER_H__ */
