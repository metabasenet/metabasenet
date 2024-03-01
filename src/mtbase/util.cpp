// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include <map>
#if defined(__linux__)
#include <sys/prctl.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#endif

#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <cstdarg>
#include <snappy.h>
#include <thread>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

namespace mtbase
{
bool STD_DEBUG = false;
void SetThreadName(const char* name)
{
#if defined(__linux__)
    ::prctl(PR_SET_NAME, name);
#elif defined(__APPLE__)
    pthread_setname_np(name);
#endif
}

std::string GetThreadName()
{
    char name[16] = { 0 };
#if defined(__linux__)
    ::prctl(PR_GET_NAME, name);
#elif defined(__APPLE__)
    pthread_getname_np(pthread_self(), name, 16);
#endif
    std::ostringstream oss;
#ifndef NDEBUG
    oss << std::hex << std::this_thread::get_id() << ":";
#endif
    oss << name;
    return oss.str();
}

#define DUMP_STACK_DEPTH_MAX 64

void PrintTrace()
{
#ifndef _WIN32
    void* stack_trace[DUMP_STACK_DEPTH_MAX] = { 0 };
    char** stack_strings = nullptr;
    int stack_depth = 0;
    int i = 0;

    stack_depth = backtrace(stack_trace, DUMP_STACK_DEPTH_MAX);

    stack_strings = (char**)backtrace_symbols(stack_trace, stack_depth);
    if (nullptr == stack_strings)
    {
        printf(" Memory is not enough while dump Stack Trace! \r\n");
        return;
    }

    printf(" Stack Trace: \r\n");
    for (i = 0; i < stack_depth; ++i)
    {
        printf(" [%d] %s \r\n", i, stack_strings[i]);
    }

    free(stack_strings);
    stack_strings = nullptr;
#endif
}

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(
    std::basic_ostream<CharT, TraitsT>& strm, severity_level lvl)
{
    static const char* const str[] = {
        "debug",
        "info",
        "warn",
        "error"
    };
    if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
        strm << str[lvl];
    else
        strm << static_cast<int>(lvl);
    return strm;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

static void console_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    logging::value_ref<severity_level> level = logging::extract<severity_level>("Severity", rec);

    auto date_time_formatter = expr::stream << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f");
    date_time_formatter(rec, strm);
    strm << "|" << logging::extract<std::string>("Channel", rec);

#if !defined(WIN32) && !defined(_WIN32)
    switch (level.get())
    {
    case debug:
        strm << "\033[0m";
        break;
    case info:
        strm << "\033[32m";
        break;
    case warn:
        strm << "\033[33m";
        break;
    case error:
        strm << "\033[31m";
        break;
    default:
        break;
    }
#endif

    strm << ":" << logging::extract<severity_level>("Severity", rec);
    strm << "|" << logging::extract<std::string>("ThreadName", rec);
    strm << "|" << rec[expr::smessage];

#if !defined(WIN32) && !defined(_WIN32)
    strm << "\033[0m";
#endif
}

typedef sinks::text_file_backend backend_t;
typedef sinks::asynchronous_sink<
    backend_t>
    sink_t;

class CBoostLog
{
public:
    CBoostLog()
    {
    }

    void Init(const boost::filesystem::path& pathData, bool debug_, bool daemon, int nLogFileSizeIn, int nLogHistorySizeIn)
    {
        sink = boost::make_shared<sink_t>(
            keywords::open_mode = std::ios::app,
            keywords::file_name = pathData / "logs" / "%Y-%m-%d_%N.log",
            keywords::rotation_size = (int64)1024 * 1024 * nLogFileSizeIn,
            keywords::auto_flush = true);

        sink->locked_backend()->set_file_collector(sinks::file::make_collector(
            keywords::target = pathData / "logh",
            keywords::max_size = (int64)1024 * 1024 * nLogHistorySizeIn,
            keywords::auto_flush = true));

        sink->locked_backend()->scan_for_files();
        sink->set_formatter(
            expr::format("%1% : [%2%] <%3%> {%4%} - %5%")
            % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            % channel
            % severity
            % expr::attr<std::string>("ThreadName")
            % expr::smessage);
        logging::core::get()->add_sink(sink);
        logging::add_common_attributes();

        typedef expr::channel_severity_filter_actor<std::string, severity_level> min_severity_filter;
        min_severity_filter min_severity = expr::channel_severity_filter(channel, severity);
        severity_level sl = debug_ ? debug : info;
        min_severity["metabasenet"] = warn;
        min_severity["CDelegate"] = warn;
        min_severity["storage"] = warn;
        auto filter = min_severity || sl <= severity;
        sink->set_filter(filter);
        if (!daemon)
        {
            typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
            boost::shared_ptr<text_sink> sink_console = boost::make_shared<text_sink>();
            boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
            sink_console->locked_backend()->add_stream(stream);
            sink_console->set_formatter(&console_formatter);
            sink_console->set_filter(filter);
            logging::core::get()->add_sink(sink_console);
        }
    }

    ~CBoostLog()
    {
        if (sink != nullptr)
        {
            sink->stop();
            sink->flush();
        }
    }
    boost::shared_ptr<sink_t> sink = nullptr;
};

static CBoostLog g_log;
static bool volatile g_log_init = false;

void StdTrace(const char* pszName, const char* pszFormat, ...)
{
    if (g_log_init && STD_DEBUG)
    {
        std::stringstream ss;
        char arg_buffer[2048] = { 0 };
        va_list ap;
        va_start(ap, pszFormat);
        vsnprintf(arg_buffer, sizeof(arg_buffer), pszFormat, ap);
        va_end(ap);
        ss << arg_buffer;
        std::string str = ss.str();

        BOOST_LOG_SCOPED_THREAD_TAG("ThreadName", GetThreadName().c_str());
        BOOST_LOG_CHANNEL_SEV(lg::get(), pszName, debug) << str;
    }
}

void StdDebug(const char* pszName, const char* pszFormat, ...)
{
    if (g_log_init && STD_DEBUG)
    {
        std::stringstream ss;
        char arg_buffer[2048] = { 0 };
        va_list ap;
        va_start(ap, pszFormat);
        vsnprintf(arg_buffer, sizeof(arg_buffer), pszFormat, ap);
        va_end(ap);
        ss << arg_buffer;
        std::string str = ss.str();

        BOOST_LOG_SCOPED_THREAD_TAG("ThreadName", GetThreadName().c_str());
        BOOST_LOG_CHANNEL_SEV(lg::get(), pszName, debug) << str;
    }
}

void StdLog(const char* pszName, const char* pszFormat, ...)
{
    if (g_log_init)
    {
        std::stringstream ss;
        char arg_buffer[2048] = { 0 };
        va_list ap;
        va_start(ap, pszFormat);
        vsnprintf(arg_buffer, sizeof(arg_buffer), pszFormat, ap);
        va_end(ap);
        ss << arg_buffer;
        std::string str = ss.str();

        BOOST_LOG_SCOPED_THREAD_TAG("ThreadName", GetThreadName().c_str());
        BOOST_LOG_CHANNEL_SEV(lg::get(), pszName, info) << str;
    }
}

void StdWarn(const char* pszName, const char* pszFormat, ...)
{
    if (g_log_init)
    {
        std::stringstream ss;
        char arg_buffer[2048] = { 0 };
        va_list ap;
        va_start(ap, pszFormat);
        vsnprintf(arg_buffer, sizeof(arg_buffer), pszFormat, ap);
        va_end(ap);
        ss << arg_buffer;
        std::string str = ss.str();

        BOOST_LOG_SCOPED_THREAD_TAG("ThreadName", GetThreadName().c_str());
        BOOST_LOG_CHANNEL_SEV(lg::get(), pszName, warn) << str;
    }
}

void StdError(const char* pszName, const char* pszFormat, ...)
{
    if (g_log_init)
    {
        std::stringstream ss;
        char arg_buffer[2048] = { 0 };
        va_list ap;
        va_start(ap, pszFormat);
        vsnprintf(arg_buffer, sizeof(arg_buffer), pszFormat, ap);
        va_end(ap);
        ss << arg_buffer;
        std::string str = ss.str();

        BOOST_LOG_SCOPED_THREAD_TAG("ThreadName", GetThreadName().c_str());
        BOOST_LOG_CHANNEL_SEV(lg::get(), pszName, error) << str;
    }
}

bool InitLog(const boost::filesystem::path& pathData, bool debug, bool daemon, int nLogFileSizeIn, int nLogHistorySizeIn)
{
    g_log_init = true;
    g_log.Init(pathData, debug, daemon, nLogFileSizeIn, nLogHistorySizeIn);
    return true;
}

bool SplitNumber(const std::string& strNumber, std::string& strInteger, std::string& strDecimal)
{
    if (strNumber.empty())
    {
        strInteger = "";
        strDecimal = "";
        return true;
    }

    int64 nValidBegin = -1;
    int64 nValidEnd = -1;
    for (std::size_t i = 0; i < strNumber.size(); i++)
    {
        if (strNumber[i] != ' ')
        {
            nValidBegin = i;
            break;
        }
    }
    if (nValidBegin == -1)
    {
        strInteger = "";
        strDecimal = "";
        return true;
    }
    for (int64 i = strNumber.size() - 1; i >= 0; i--)
    {
        if (strNumber[i] != ' ')
        {
            nValidEnd = i + 1;
            break;
        }
    }
    if (nValidEnd == -1 || nValidEnd <= nValidBegin)
    {
        return false;
    }

    std::string strValidNumber;
    strValidNumber.assign(&(strNumber[nValidBegin]), nValidEnd - nValidBegin);

    int64 nDot = -1;
    for (std::size_t i = 0; i < strValidNumber.size(); i++)
    {
        char c = strValidNumber[i];
        if (!((c >= '0' && c <= '9') || c == '.'))
        {
            return false;
        }
        if (c == '.')
        {
            if (nDot >= 0)
            {
                return false;
            }
            nDot = i;
        }
    }
    if (nDot < 0)
    {
        strInteger = strValidNumber;
        strDecimal = "";
    }
    else
    {
        if (nDot == 0)
        {
            strInteger = "";
        }
        else
        {
            strInteger = strValidNumber.substr(0, nDot);
        }
        int64 nDecCount = strValidNumber.size() - (nDot + 1);
        if (nDecCount > 0)
        {
            strDecimal = strValidNumber.substr(nDot + 1, nDecCount);
        }
        else
        {
            strDecimal = "";
        }
    }
    return true;
}

bool BtCompress(const bytes& btSrc, bytes& btDst)
{
    bytes btOut;
    size_t nNeedDstBufSize = snappy::MaxCompressedLength(btSrc.size());
    btOut.resize(nNeedDstBufSize);
    size_t dstlen = nNeedDstBufSize;
    snappy::RawCompress((const char*)btSrc.data(), btSrc.size(), (char*)btOut.data(), &dstlen);
    btDst.assign(btOut.data(), btOut.data() + dstlen);
    return true;
}

bool BtUncompress(const bytes& btSrc, bytes& btDst)
{
    size_t ulength = 0;
    if (!snappy::GetUncompressedLength((char*)btSrc.data(), btSrc.size(), &ulength))
    {
        return false;
    }
    btDst.resize(ulength);
    if (!snappy::RawUncompress((char*)btSrc.data(), btSrc.size(), (char*)btDst.data()))
    {
        return false;
    }
    return true;
}

} // namespace mtbase