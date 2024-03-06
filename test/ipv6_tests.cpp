// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/test/unit_test.hpp>
#include <exception>

#include "mtbase.h"
#include "netio/iocontainer.h"
#include "netio/nethost.h"
#include "test_big.h"

using namespace mtbase;

BOOST_FIXTURE_TEST_SUITE(ipv6_tests, BasicUtfSetup)

class CIOInBoundTest : public CIOInBound
{
public:
    CIOInBoundTest()
      : CIOInBound(new CIOProc("test")) {}
    void TestIPv6()
    {
        try
        {
            std::vector<std::string> vAllowMask{ "192.168.199.*", "2001:db8:0:f101::*",
                                                 "fe80::95c3:6a70:f4e4:5c2a", "fe80::95c3:6a70:*:fc2a" };
            if (!BuildWhiteList(vAllowMask))
            {
                BOOST_FAIL("BuildWhiteList return false");
            }

            int tempPort = 10;

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("192.168.199.1"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == true);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("192.168.196.34"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == false);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("2001:db8:0:f101::1"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == true);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("2001:db8:0:f102::23"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == false);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("::1"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == false);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("fe80::95c3:6a70:f4e4:5c2a"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == true);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("fe80::95c3:6a70:f4e4:5c2f"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == false);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("fe80::95c3:6a70:a231:fc2a"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == true);
            }

            {
                boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("fe80::95c3:6a70:a231:fc21"), tempPort);
                BOOST_CHECK(IsAllowedRemote(ep) == false);
            }
        }
        catch (const std::exception& exp)
        {
            BOOST_FAIL("Regex Error.");
        }
    }
};

BOOST_AUTO_TEST_CASE(white_list_regex)
{
    CIOInBoundTest ioInBoundTest;
    ioInBoundTest.TestIPv6();
}

void ParseIpPortString(const std::string& strIpPort, std::string& strIp, uint16& nPort)
{
    std::string endpointString = strIpPort;
    endpointString.erase(std::remove_if(endpointString.begin(), endpointString.end(), ::isspace), endpointString.end());
    if (endpointString.empty())
    {
        strIp = "";
        nPort = 0;
        return;
    }

    std::string portString;
    if (endpointString[0] == '[')
    {
        std::regex ipv6Regex("\\[([0-9a-fA-F:]+)\\]:(\\d+)");
        std::smatch match;
        if (std::regex_match(endpointString, match, ipv6Regex))
        {
            if (match.size() == 3)
            {
                strIp = match[1].str();
                portString = match[2].str();
            }
            else
            {
                strIp = "";
                portString = "";
            }
        }
    }
    else
    {
        std::regex ipv4Regex("([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+):(\\d+)");
        std::smatch match;
        if (std::regex_match(endpointString, match, ipv4Regex))
        {
            if (match.size() == 3)
            {
                strIp = match[1].str();
                portString = match[2].str();
            }
            else
            {
                strIp = "";
                portString = "";
            }
        }
    }
    if (portString.empty())
    {
        nPort = 0;
    }
    else
    {
        try
        {
            nPort = std::stoi(portString);
        }
        catch (const std::invalid_argument& e)
        {
            nPort = 0;
        }
        catch (const std::out_of_range& e)
        {
            nPort = 0;
        }
    }
}

BOOST_AUTO_TEST_CASE(ipv6parser)
{
    std::string endpointString;
    std::string strIp;
    uint16 nPort = 0;

    endpointString = "86.23.44.12:8080";
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV4 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;

    endpointString = "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:8080";
    strIp = "";
    nPort = 0;
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV6 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;

    endpointString = "  86.23.44.12 :  8080 ";
    strIp = "";
    nPort = 0;
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV4 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;

    endpointString = "  [ 2001: 0db8:85a3  :0000:0000:8a2e:0370:7334] :  8080";
    strIp = "";
    nPort = 0;
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV6 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;

    endpointString = "[::1]:631";
    strIp = "";
    nPort = 0;
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV6 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;

    endpointString = "86.23.44.12";
    strIp = "";
    nPort = 0;
    ParseIpPortString(endpointString, strIp, nPort);
    std::cout << "IPV4 Address: " << strIp << std::endl;
    std::cout << "Port: " << nPort << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
