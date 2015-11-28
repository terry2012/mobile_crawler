#ifndef __PUBLIC_STRING_POOL_H__
#define __PUBLIC_STRING_POOL_H__ 1

#include "common.hpp"

typedef uint64_t string_hash_t;

template <typename StringArray = deque<string>,
          typename StringHash = string_hash_t,
          typename Index = uint64_t>
class PublicStringPool {
private:
        std::unordered_map<StringHash, Index> hash_to_index;
        StringArray string_pool;

        PublicStringPool() {
                read_files();
        }

        int read_files() {
                ifstream input_file(STRINGS_FILE);
                string line;

                if (not input_file.is_open()) {
                        return -1;
                }

                while (not input_file.eof()) {
                        line.clear();
                        getline(input_file, line);
                        trim(line);

                        if (line.length() == 0 || *(line.begin()) == '#')
                                continue;

                        add_string(&line);
                }

                input_file.close();

                return 0;
        }


public:
        static PublicStringPool& get_instance() {
                static PublicStringPool instance;
                return instance;
        }

        ~PublicStringPool() {
                ;
        }

        const string* get_string_from_hash(StringHash sh) {
                if (hash_to_index.count(sh) != 0)
                        return &string_pool[hash_to_index[sh]];

                static string empty_str("");
                return &empty_str;
        }

        bool exist(string* str) {
                if (NULL == str)
                        return false;

                return exist(myhash(str));
        }

        bool exist(StringHash sh) {
                return hash_to_index.count(sh);
        }


        StringHash add_string(string* str) {
                if (NULL == str)
                        return 0;

                StringHash sh = myhash(str);
                if (exist(str))
                        return sh;

                string_pool.push_back(*str);
                hash_to_index[sh] = string_pool.size() - 1;
                return sh;
        }

        StringHash get_hash(string *str) {
                if (NULL == str)
                        return 0;

                return myhash(str);
        }

        int end() {
                ofstream fd(STRINGS_FILE, ios::out | ios::trunc);

                auto it = string_pool.begin();
                while (it != string_pool.end()) {
                        fd << *it;
                        fd << "\n";

                        it++;
                }

                fd.close();

                return 0;
        }
};


#endif /* __PUBLIC_STRING_POOL_H__ */
