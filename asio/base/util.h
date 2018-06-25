#ifndef ASIO_BASE_UTIL_H_
#define ASIO_BASE_UTIL_H_

#include <string>

namespace util {

void initlog(std::string path, bool console = false);
time_t currentMillisecond();
std::string getFormatTime(time_t t);

}  // namespace util

#endif  // ASIO_BASE_UTIL_H_
