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

    // Euro symbol.
    {
        stringstream result;
        encode(result, string("H\xe2\x82\xac"));
        BOOST_CHECK(result.str() == string("\x18H\xe2\x82\xac\x00", 6));
    }

    {
        stringstream result;
        BOOST_CHECK_THROW(encode(result, string("H\x00llo", 5)), out_of_range);
    }
}

