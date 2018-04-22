#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "RSONDecode tests"
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <boost/none.hpp>
#include "RSONDecode.hpp"

using namespace std;
using namespace boost;
using namespace Orion::Rigel::RSON;

BOOST_AUTO_TEST_CASE(DecodeString)
{
    BOOST_CHECK(decode<string>(string("\x18\x00", 2))              == string(""));
    BOOST_CHECK(decode<string>(string("\x18H\x00", 3))             == string("H"));
    BOOST_CHECK(decode<string>(string("H\xe5", 2))                 == string("He"));
    BOOST_CHECK(decode<string>(string("Hello Worl\xe4"))           == string("Hello World"));
    BOOST_CHECK(decode<string>(string("\x18H\xe2\x82\xac\x00", 6)) == string("H\xe2\x82\xac"));
}

BOOST_AUTO_TEST_CASE(DecodeInt8)
{
    BOOST_CHECK(decode<int8_t>(string("\xc0", 1))     == 0);
    BOOST_CHECK(decode<int8_t>(string("\xc1", 1))     == 1);
    BOOST_CHECK(decode<int8_t>(string("\xff", 1))     == -1);
    BOOST_CHECK(decode<int8_t>(string("\xdf", 1))     == 31);
    BOOST_CHECK(decode<int8_t>(string("\xe0", 1))     == -32);
    BOOST_CHECK(decode<int8_t>(string("\xbf\x80", 2)) == 63);
    BOOST_CHECK(decode<int8_t>(string("\x80\xff", 2)) == -64);
    BOOST_CHECK(decode<int8_t>(string("\xbf\x81", 2)) == 127);
    BOOST_CHECK(decode<int8_t>(string("\x80\xfe", 2)) == -128);
}

BOOST_AUTO_TEST_CASE(DecodeUInt8)
{
    BOOST_CHECK(decode<uint8_t>(string("\xc0", 1))     == 0);
    BOOST_CHECK(decode<uint8_t>(string("\xc1", 1))     == 1);
    BOOST_CHECK(decode<uint8_t>(string("\xdf", 1))     == 31);
    BOOST_CHECK(decode<uint8_t>(string("\xbf\x80", 2)) == 63);
    BOOST_CHECK(decode<uint8_t>(string("\xbf\x81", 2)) == 127);
    BOOST_CHECK(decode<uint8_t>(string("\xbf\x83", 2)) == 255);
}

BOOST_AUTO_TEST_CASE(DecodeBoolean)
{
    BOOST_CHECK_NO_THROW(decode<boost::none_t>(string("\x10", 1)));
    BOOST_CHECK(decode<bool>(string("\x11", 1)) == true);
    BOOST_CHECK(decode<bool>(string("\x12", 1)) == false);
}

BOOST_AUTO_TEST_CASE(DecodeDouble)
{
    BOOST_CHECK(decode<double>(string("\x16\xc0", 2))     == 0.0);
    BOOST_CHECK(decode<double>(string("\x16\xc1\xc0", 3)) == 1.0);
    BOOST_CHECK(decode<double>(string("\x16\xc1\xc1", 3)) == 2.0);
    BOOST_CHECK(decode<double>(string("\x16\xc1\xff", 3)) == 0.5);
    BOOST_CHECK(decode<double>(string("\x16\xc3\xc1", 3)) == 6.0);
    BOOST_CHECK(decode<double>(string("\x16\xd3\xc1", 3)) == 38.0);
}

BOOST_AUTO_TEST_CASE(DecodeVector)
{
    {
        auto v = decode<vector<any>>(string("\x13" "fo\xef" "ba\xf2" "\x00", 8));
        BOOST_CHECK(v.size() == 2);
        BOOST_CHECK(any_cast<string>(v[0]) == "foo");
        BOOST_CHECK(any_cast<string>(v[1]) == "bar");
    }

    {
        auto v = decode<vector<any>>(string("\x13" "\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0" "\x14" "fo\xef" "\x00" "\xc0" "\x00", 18));
        BOOST_CHECK(v.size() == 2);

        {
            auto m = any_cast<map<string, any>>(v[0]);
            BOOST_CHECK(m.size() == 2);
            BOOST_CHECK(any_cast<int64_t>(m["foo"]) == 0);
            BOOST_CHECK(any_cast<int64_t>(m["bar"]) == 1);
        }

        {
            auto m = any_cast<map<string, any>>(v[1]);
            BOOST_CHECK(m.size() == 1);
            BOOST_CHECK(any_cast<int64_t>(m["foo"]) == 0);
        }
    }
}

BOOST_AUTO_TEST_CASE(DecodeMap)
{
    {
        auto m = decode<map<string, any>>(string("\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0", 10));
        BOOST_CHECK(m.size() == 2);
        BOOST_CHECK(any_cast<int64_t>(m["foo"]) == 0);
        BOOST_CHECK(any_cast<int64_t>(m["bar"]) == 1);
    }
}

