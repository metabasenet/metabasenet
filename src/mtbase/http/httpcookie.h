// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_HTTP_HTTPCOOKIE_H
#define MTBASE_HTTP_HTTPCOOKIE_H

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <map>
#include <string>

namespace mtbase
{

class CHttpCookie
{
public:
    static const boost::posix_time::ptime INVALID_PTIME;
    CHttpCookie();
    CHttpCookie(const std::string& strFromResp);
    CHttpCookie(const std::string& strNameIn, const std::string& strValueIn,
                const std::string& strDomainIn = "",
                const std::string& strPathIn = "",
                const boost::posix_time::ptime& ptExpiresIn = INVALID_PTIME,
                bool fSecureIn = false, bool fHttpOnlyIn = false);
    ~CHttpCookie();
    bool IsNull();
    bool IsPersistent();
    bool LoadFromResp(const std::string& strFromResp);
    void Delete();
    const std::string BuildSetCookie();

protected:
    void SetNull();

public:
    std::string strName;
    std::string strValue;
    std::string strDomain;
    std::string strPath;
    boost::posix_time::ptime ptExpires;
    bool fSecure;
    bool fHttpOnly;
};

} // namespace mtbase
#endif //MTBASE_HTTP_HTTPCOOKIE_H
