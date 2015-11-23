#ifndef __MYLOG_H__
#define __MYLOG_H__ 1

#include "common.hpp"


#define LOGD(fmt, ...) printf("%s  DEBUG    (%s:%d)\t" fmt "\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...) printf("%s  INFO     (%s:%d)\t" fmt "\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...) printf("%s  WARN     (%s:%d)\t" fmt "\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("%s  ~!ERR!~  (%s:%d)\t" fmt " \t[errno]: %s\n", __TIME__, __FILE__, __LINE__, ##__VA_ARGS__, strerror(errno))


#endif /* __MYLOG_H__ */
