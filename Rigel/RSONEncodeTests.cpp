/* Copyright 2018 Tjienta Vara
 * This file is part of Orion.
 *
 * Orion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orion.  If not, see <http://www.gnu.org/licenses/>.
 */
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "RSONEncode tests"
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <boost/none.hpp>
#include "RSONEncode.hpp"

using namespace std;
using namespace boost;
using namespace Orion::Rigel::RSON;

BOOST_AUTO_TEST_CASE(EncodeString)
{
    BOOST_CHECK(encode(string("")) == string("\x18\x00", 2));
    BOOST_CHECK(encode(string("H")) == string("H\x80", 2));
    BOOST_CHECK(encode(string("He")) == string("H\xe5", 2));
    BOOST_CHECK(encode(string("Hello World")) == "Hello Worl\xe4");
    BOOST_CHECK(encode(string("H\xe2\x82\xac")) == string("\x18H\xe2\x82\xac\x00", 6));
    BOOST_CHECK_THROW(encode(string("H\x00llo", 5)), encode_value_error);
}

BOOST_AUTO_TEST_CASE(EncodeInt8)
{
    BOOST_CHECK(encode(static_cast<int8_t>(0)) == string("\xc0", 1));
    BOOST_CHECK(encode(static_cast<int8_t>(1)) == string("\xc1", 1));
    BOOST_CHECK(encode(static_cast<int8_t>(-1)) == string("\xff", 1));
    BOOST_CHECK(encode(static_cast<int8_t>(31)) == string("\xdf", 1));
    BOOST_CHECK(encode(static_cast<int8_t>(-32)) == string("\xe0", 1));
    BOOST_CHECK(encode(static_cast<int8_t>(63)) == string("\xbf\x80", 2));
    BOOST_CHECK(encode(static_cast<int8_t>(-64)) == string("\x80\xff", 2));
    BOOST_CHECK(encode(static_cast<int8_t>(127)) == string("\xbf\x81", 2));
    BOOST_CHECK(encode(static_cast<int8_t>(-128)) == string("\x80\xfe", 2));
}

BOOST_AUTO_TEST_CASE(EncodeUInt8)
{
    BOOST_CHECK(encode(static_cast<uint8_t>(0)) == string("\xc0", 1));
    BOOST_CHECK(encode(static_cast<uint8_t>(1)) == string("\xc1", 1));
    BOOST_CHECK(encode(static_cast<uint8_t>(31)) == string("\xdf", 1));
    BOOST_CHECK(encode(static_cast<uint8_t>(63)) == string("\xbf\x80", 2));
    BOOST_CHECK(encode(static_cast<uint8_t>(127)) == string("\xbf\x81", 2));
    BOOST_CHECK(encode(static_cast<uint8_t>(255)) == string("\xbf\x83", 2));
}

BOOST_AUTO_TEST_CASE(EncodeFloat)
{
    BOOST_CHECK(encode(static_cast<float>(0.0)) == string("\x16\xc0", 2));
    BOOST_CHECK(encode(static_cast<float>(1.0)) == string("\x16\xc1\xc0", 3));
    BOOST_CHECK(encode(static_cast<float>(2.0)) == string("\x16\xc1\xc1", 3));
    BOOST_CHECK(encode(static_cast<float>(0.5)) == string("\x16\xc1\xff", 3));
    BOOST_CHECK(encode(static_cast<float>(6.0)) == string("\x16\xc3\xc1", 3));
    BOOST_CHECK(encode(static_cast<float>(38.0)) == string("\x16\xd3\xc1", 3));
}

BOOST_AUTO_TEST_CASE(EncodeDouble)
{
    BOOST_CHECK(encode(static_cast<double>(0.0)) == string("\x16\xc0", 2));
    BOOST_CHECK(encode(static_cast<double>(1.0)) == string("\x16\xc1\xc0", 3));
    BOOST_CHECK(encode(static_cast<double>(2.0)) == string("\x16\xc1\xc1", 3));
    BOOST_CHECK(encode(static_cast<double>(0.5)) == string("\x16\xc1\xff", 3));
    BOOST_CHECK(encode(static_cast<double>(6.0)) == string("\x16\xc3\xc1", 3));
    BOOST_CHECK(encode(static_cast<double>(38.0)) == string("\x16\xd3\xc1", 3));
}

BOOST_AUTO_TEST_CASE(EncodeBoolean)
{
    BOOST_CHECK(encode(none) == string("\x10", 1));
    BOOST_CHECK(encode(true) == string("\x11", 1));
    BOOST_CHECK(encode(false) == string("\x12", 1));
}

BOOST_AUTO_TEST_CASE(EncodeVector)
{
    BOOST_CHECK(encode(vector<string>{"foo", "bar"}) == string("\x13" "fo\xef" "ba\xf2" "\x00", 8));
    BOOST_CHECK(encode(vector<map<string, int>>{{{"foo", 0}, {"bar", 1}}, {{"foo", 0}}}) == string("\x13" "\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0" "\x14" "fo\xef" "\x00" "\xc0" "\x00", 18));
}

BOOST_AUTO_TEST_CASE(EncodeMap)
{
    BOOST_CHECK(encode(map<string, int>{{"foo", 0}, {"bar", 1}}) == string("\x14" "ba\xf2" "fo\xef" "\x00" "\xc1" "\xc0", 10));
}


