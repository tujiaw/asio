#pragma once
#include <string>
#include <memory>

namespace ningto
{
    namespace setting {
        void initlog(std::string path, bool console = false);
        void initEnableCompressSize(int size = 1024);
        int enableCompressSize();
    }

    namespace util {
        time_t currentMillisecond();
        std::string getFormatTime(time_t t);
        std::string bytesFormat(std::size_t size);
        std::string getFormatRss();
        void sleep(int millisecond);
        std::string readFile(const std::string &path);
        template<typename T, typename... Ts>
        std::unique_ptr<T> make_unique(Ts&&... params) { return std::unique_ptr<T>(new T(std::forward<Ts>(params)...)); } // c++14
    }  // namespace util
}
