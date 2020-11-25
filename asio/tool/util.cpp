#include "util.h"

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <boost/date_time.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include "internal/Package.h"
#include "internal/ProtoHelp.h"
#include "process_memory.h"

namespace ningto
{
    static const boost::posix_time::ptime EPOCH(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
    namespace setting {
        void failureWriter(const char *data, int size) {
            LOG(ERROR) << std::string(data, size);
        }

        void initlog(std::string path, bool console) {
            google::InstallFailureWriter(failureWriter);
            const std::string fileNamePrefix = "log";
            std::replace(path.begin(), path.end(), '\\', '/');
            google::InitGoogleLogging(path.c_str());
            std::string dir = path.substr(0, path.find_last_of('/') + 1) + "log/";
#ifdef WIN32
            _mkdir(dir.c_str());
#else
            mkdir(dir.c_str(), 0755);
#endif
            google::SetLogDestination(google::GLOG_INFO, (dir + fileNamePrefix).c_str());
            FLAGS_alsologtostderr = console;
        }

        void initEnableCompressSize(int size) {
            ProtoHelp::initEnableCompressSize(size);
        }

        int enableCompressSize() {
            return ProtoHelp::enableCompressSize();
        }
    }

    namespace util {
        time_t currentMillisecond() {
            return (boost::posix_time::microsec_clock::universal_time() - EPOCH).total_milliseconds();
        }

        std::string getFormatTime(time_t t) {
            if (t < 0) {
                return "";
            }

            if (t > 1528959332000) {
                t /= 1000;
            }
            struct tm *local;
            struct tm tmpLocal;
#ifdef WIN32
            // 目前的版本windows是线程安全的
            local = localtime(&t);
#else
            // linux下localtime_r是线程安全的, localtime线程不安全
            local = localtime_r(&t, &tmpLocal);
#endif
            char timeChars[50];
            strftime(timeChars, 50, "%Y-%m-%d %H:%M:%S", local);
            return std::string(timeChars);
        }

        std::string bytesFormat(std::size_t size)
        {
            std::stringstream ss;
            if (size < 1024) {
                ss << size << " B";
            } else if (size < 1024 * 1024) {
                ss << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (size * 1.0 / 1024) << " KB";
            } else if (size < 1024 * 1024 * 1024) {
                ss << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (size * 1.0 / 1024 / 1024) << " MB";
            } else {
                ss << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (size * 1.0 / 1024 / 1024 / 1024) << " GB";
            }
            return ss.str();
        }

        std::string getFormatRss()
        {
            return bytesFormat(getCurrentRSS());
        }

        void sleep(int millisecond)
        {
#ifdef WIN32
            ::Sleep(millisecond);
#else
            usleep(1000 * millisecond);
#endif
        }

        std::string readFile(const std::string &path)
        {
            try {
                std::ifstream ifs(path);
                std::stringstream ss;
                ss << ifs.rdbuf();
                return ss.str();
            } catch (std::ifstream::failure e) {
                std::cerr << "Exception read file, ifstream error:" << e.what();
                return "";
            } catch (std::exception e) {
                std::cerr << "Exception read file, error:" << e.what();
                return "";
            }
        }
    }  // namespace util
}
