#ifndef __MYGRAPH_H__
#define __MYGRAPH_H__ 1

#include "common.hpp"
#include "public_string_pool.hpp"

typedef struct RealVertex {
        string_hash_t  url;
        string_hash_t  file_path;
        time_t  crawled_time;
        short  status_code;
        bool  is_crawled;
        bool  is_malicious;
} MyVertex;

typedef boost::adjacency_list<listS, vecS, directedS, MyVertex>  RealGraph;
template < typename Graph = RealGraph,
           typename Vertex = RealVertex,
           typename VD  = typename boost::graph_traits<Graph>::vertex_descriptor,
           typename ED  = typename boost::graph_traits<Graph>::edge_descriptor,
           typename StringHash = string_hash_t,
           typename UrlToVertex  = typename std::unordered_map<StringHash, VD> >
class MyGraph {
private:
        UrlToVertex  m_urls_to_vertices;
        string  m_graph_file_path;
        Graph  m_graph;

private:
        MyGraph(string graph_file_path) : m_graph_file_path(graph_file_path) {
                read_graph();

                typedef typename graph_traits<Graph>::vertex_iterator vertex_it;
                vertex_it it, it_end;
                boost::tie(it, it_end) = vertices(m_graph);
                while (it != it_end) {
                        m_urls_to_vertices[m_graph[*it].url] = *it;
                        it += 1;
                }
        }

public:
        ~MyGraph() {
                ;
        }

        static MyGraph& get_instance(string graph_file_path) {
                static MyGraph my_graph(graph_file_path);
                return my_graph;
        }

        Graph& get_graph() {
                return m_graph;
        }

        bool is_crawled(StringHash url) {
                if (m_urls_to_vertices.count(url) == 0)
                        return false;

                VD v = m_urls_to_vertices[url];
                return m_graph[v].is_crawled;
        }

        void init_vertex(RealVertex* v, StringHash url, StringHash file_path, time_t crawled_time,
                         int status_code, bool is_crawled, bool is_malicious) {
                v->url = url;
                v->file_path = file_path;
                v->crawled_time = crawled_time;
                v->status_code = status_code;
                v->is_crawled = is_crawled;
                v->is_malicious = is_malicious;
        }

        VD get_vd(StringHash url) {
                VD v;
                if (m_urls_to_vertices.count(url) == 0) {
                        v = boost::add_vertex(m_graph);
                        m_urls_to_vertices[url] = v;

                        init_vertex(&m_graph[v], url, 0, 0, 0, false, false);
                } else {
                        v = m_urls_to_vertices[url];
                }

                return v;
        }


        int add_vertex(StringHash url, StringHash file_path, time_t crawled_time,
                       int status_code, bool is_crawled, bool is_malicious) {
                VD v = get_vd(url);
                init_vertex(&m_graph[v], url, file_path, crawled_time,
                            status_code, is_crawled, is_malicious);
                return 0;
        }

        int add_edge(StringHash src_url, StringHash dst_url) {
                VD src_vd = get_vd(src_url);
                VD dst_vd = get_vd(dst_url);
                boost::add_edge(src_vd, dst_vd, m_graph);
                return 0;
        }

        dynamic_properties get_dp() {
                boost::dynamic_properties dp(ignore_other_properties);
                dp.property("url", get(&Vertex::url, m_graph));
                dp.property("file_path", get(&Vertex::file_path, m_graph));
                dp.property("crawled_time", get(&Vertex::crawled_time, m_graph));
                dp.property("status_code", get(&Vertex::status_code, m_graph));
                dp.property("is_crawled", get(&Vertex::is_crawled, m_graph));
                dp.property("is_malicious", get(&Vertex::is_malicious, m_graph));

                return dp;
        }

        int read_graph() {
                ifstream myfile(GRAPH_FILE);
                if (myfile.is_open()) {
                        dynamic_properties dp = get_dp();
                        try {
                                boost::read_graphml(myfile, m_graph, dp, true);
                        } catch (const std::exception& ex) {
                                LOGE("@read_graphml@ Can't read %s.  reason: %s", GRAPH_FILE, ex.what());
                        }

                        myfile.close();
                } else
                        LOGE("Can't open %s", GRAPH_FILE);

                return 0;
        }

        int write_graph() {
                ofstream myfile;
                myfile.open(GRAPH_FILE, ios::out | ios::trunc);
                dynamic_properties dp = get_dp();
                write_graphml(myfile, m_graph, dp, true);
                myfile.close();

                return 0;
        }

        int end() {
                this->write_graph();
                return 0;
        }
};

#endif /* __MYGRAPH_H__ */
