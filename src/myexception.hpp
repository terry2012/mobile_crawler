#ifndef __MYEXCEPTION_H__
#define __MYEXCEPTION_H__ 1

using namespace std;

class MyException: public std::exception
{
private:
        string m_msg;

public:
        MyException() {
                m_msg = "unkown error";
        }

        MyException(string m) {
                m_msg = m;
        }

        MyException(const char* m) {
                m_msg = string(m);
        }

        ~MyException() throw() {}

        const char* what() const throw() {
                static string tmp_str;
                tmp_str = m_msg + string("\t\t[errno] ") + string(strerror(errno));
                return tmp_str.c_str();
        }
};

#endif /* __MYEXCEPTION_H__ */
