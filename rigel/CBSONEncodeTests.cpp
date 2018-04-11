#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "CBSONEncode tests"
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <CBSONEncodeLength.hpp>
#include <CBSONEncode.hpp>

using namespace std;
using namespace Orion::Rigel::CBSON;

BOOST_AUTO_TEST_CASE(EncodeString)
{
    {
        stringstream result;
        encode(result, string(""));
        BOOST_CHECK(result.str() == string("\x18\x00", 2));
    }

    {
        stringstream result;
        encode(result, string("H"));
        BOOST_CHECK(result.str() == string("\x18H\x00", 3));
    }

    {
        stringstream result;
        encode(result, string("He"));
        BOOST_CHECK(result.str() == string("H\xe5", 2));
    }

    {
        stringstream result;
        encode(result, string("Hello World"));
        BOOST_CHECK(result.str() == "Hello Worl\xe4");
    }

    {   // Euro symbol.
        stringstream result;
        encode(result, string("H\xe2\x82\xac"));
        BOOST_CHECK(result.str() == string("\x18H\xe2\x82\xac\x00", 6));
    }

    {
        stringstream result;
        BOOST_CHECK_THROW(encode(result, string("H\x00llo", 5)), out_of_range);
    }
}

BOOST_AUTO_TEST_CASE(EncodeInt8)
{
    {
        stringstream result;
        encode(result, static_cast<int8_t>(0));
        BOOST_CHECK(result.str() == string("\xc0", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(1));
        BOOST_CHECK(result.str() == string("\xc1", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(-1));
        BOOST_CHECK(result.str() == string("\xff", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(31));
        BOOST_CHECK(result.str() == string("\xdf", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(-32));
        BOOST_CHECK(result.str() == string("\xe0", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(63));
        BOOST_CHECK(result.str() == string("\xbf\x80", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(-64));
        BOOST_CHECK(result.str() == string("\x80\xff", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(127));
        BOOST_CHECK(result.str() == string("\xbf\x81", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<int8_t>(-128));
        BOOST_CHECK(result.str() == string("\x80\xfe", 2));
    }
}

BOOST_AUTO_TEST_CASE(EncodeUInt8)
{
    {
        stringstream result;
        encode(result, static_cast<uint8_t>(0));
        BOOST_CHECK(result.str() == string("\xc0", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<uint8_t>(1));
        BOOST_CHECK(result.str() == string("\xc1", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<uint8_t>(31));
        BOOST_CHECK(result.str() == string("\xdf", 1));
    }

    {
        stringstream result;
        encode(result, static_cast<uint8_t>(63));
        BOOST_CHECK(result.str() == string("\xbf\x80", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<uint8_t>(127));
        BOOST_CHECK(result.str() == string("\xbf\x81", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<uint8_t>(255));
        BOOST_CHECK(result.str() == string("\xbf\x83", 2));
    }
}

BOOST_AUTO_TEST_CASE(EncodeFloat)
{
    {
        stringstream result;
        encode(result, static_cast<float>(0.0));
        BOOST_CHECK(result.str() == string("\x16\xc0", 2));
    }

    {
        stringstream result;
        encode(result, static_cast<float>(1.0));
        BOOST_CHECK(result.str() == string("\x16\xc1\xc0", 3));
    }

    {
        stringstream result;
        encode(result, static_cast<float>(2.0));
        BOOST_CHECK(result.str() == string("\x16\xc1\xc1", 3));
    }

    {
        stringstream result;
        encode(result, static_cast<float>(0.5));
        BOOST_CHECK(result.str() == string("\x16\xc1\xff", 3));
    }

    {
        stringstream result;
        encode(result, static_cast<float>(6.0));
        BOOST_CHECK(result.str() == string("\x16\xc3\xc1", 3));
    }

    {
        stringstream result;
        encode(result, static_cast<float>(38.0));
        BOOST_CHECK(result.str() == string("\x16\xd3\xc1", 3));
    }
}

BOOST_AUTO_TEST_CASE(EncodeBoolean)
{
    {
        stringstream result;
        encode(result, none);
        BOOST_CHECK(result.str() == string("\x10", 1));
    }

    {
        stringstream result;
        encode(result, true);
        BOOST_CHECK(result.str() == string("\x11", 1));
    }

    {
        stringstream result;
        encode(result, false);
        BOOST_CHECK(result.str() == string("\x12", 1));
    }

}

BOOST_AUTO_TEST_CASE(EncodeVector)
{
    {
        stringstream result;
        encode(result, vector<string>{"foo", "bar"});
        BOOST_CHECK(result.str() == string("\x13" "fo\xef" "ba\xf2" "\x00", 8));
    }

    {
        stringstream result;
        encode(result, vector<map<string, int>>{{{"foo", 0}, {"bar", 1}}, {{"foo", 0}}});
        BOOST_CHECK(result.str() == string("\x13" "\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0" "\x00" "\x14" "fo\xef" "\x00" "\xc0" "\x00" "\x00", 20));
    }
}

BOOST_AUTO_TEST_CASE(EncodeMap)
{
    {   // The result must be sorted.
        stringstream result;
        encode(result, map<string, int>{{"foo", 0}, {"bar", 1}});
        BOOST_CHECK(result.str() == string("\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0" "\x00", 11));
    }
}


