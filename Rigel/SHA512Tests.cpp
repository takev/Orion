#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SHA512
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include "SHA512.hpp"

using namespace std;
using namespace boost;
using namespace Orion::Rigel;

BOOST_AUTO_TEST_CASE(TestSHA512)
{
    BOOST_CHECK_EQUAL(SHA512("abc").finish(), BigInt<512>("0xddaf35a193617aba cc417349ae204131 12e6fa4e89a97ea2 0a9eeee64b55d39a 2192992a274fc1a8 36ba3c23a3feebbd 454d4423643ce80e 2a9ac94fa54ca49f"));
    BOOST_CHECK_EQUAL(SHA512("").finish(), BigInt<512>("0xcf83e1357eefb8bd f1542850d66d8007 d620e4050b5715dc 83f4a921d36ce9ce 47d0d13c5d85f2b0 ff8318d2877eec2f 63b931bd47417a81 a538327af927da3e"));
    BOOST_CHECK_EQUAL(SHA512("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq").finish(), BigInt<512>("0x204a8fc6dda82f0a 0ced7beb8e08a416 57c16ef468b228a8 279be331a703c335 96fd15c13b1b07f9 aa1d3bea57789ca0 31ad85c7a71dd703 54ec631238ca3445"));

    //                                 1         2         3         4         5         6         7         8         9         0         1         2
    //                        123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0x9814d48ae1bfd731b32f0a829f20507ec9bd6b77609053718f7e2053b53c7a264bbab6a96d3d54a7f9a736570d11b1f99afb1735149f43cfee9b6f87886d3ff6"));
    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0xc1b0f5c6d3b03dfe4a2602e67242f54e344090b66e01100a469b129f583f016c7e27dddeaa438393dcc7ec54b0b57c9ba7af007f9b56db5f6fb677d972a31362"));
    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0x01d35c10c6c38c2dcf48f7eebb3235fb5ad74a65ec4cd016e2354c637a8fb49b695ef3c1d6f7ae4cd74d78cc9c9bcac9d4f23a73019998a7f73038a5c9b2dbde"));
    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0xb83086cd8494e55708ad7ecd82dfb4bca1bda61ecbb7caf0c68967902e709345e5d8305eb7ac0d588afc6cbb75161aa9c8c7e0ea986bd833dafe5e1ccd37345a"));

    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0xfa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef86818196921760b4beff48404df811b953828274461673c68d04e297b0eb7b2b4d60fc6b566a2"));

    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0xc01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32bd05f0f1ba33e568b88fd2d970929b719ecbb152f58f130a407c8830604b70ca"));

    BOOST_CHECK_EQUAL(SHA512("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").finish(), BigInt<512>("0x55ddd8ac210a6e18ba1ee055af84c966e0dbff091c43580ae1be703bdb85da31acf6948cf5bd90c55a20e5450f22fb89bd8d0085e39f85a86cc46abbca75e24d"));

    BOOST_CHECK_EQUAL(SHA512("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu").finish(), BigInt<512>("0x8e959b75dae313da 8cf4f72814fc143f 8f7779c6eb9f7fa1 7299aeadb6889018 501d289e4900f7e4 331b99dec4b5433a c7d329eeb6dd2654 5e96e55b874be909"));

    {
        // one million (1,000,000) repetitions of the character "a" (0x61).
        auto H = SHA512();
        for (int i = 0; i < 20000; i++) {
            //     12345678901234567890123456789012345678901234567890
            H.add("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        }
        BOOST_CHECK_EQUAL(H.finish(), BigInt<512>("0xe718483d0ce76964 4e2e42c7bc15b463 8e1f98b13b204428 5632a803afa973eb de0ff244877ea60a 4cb0432ce577c31b eb009c5c2c49aa2e 4eadb217ad8cc09b"));
    }

    {
        // the extremely-long message "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
        // repeated 16,777,216 times: a bit string of length 233 bits (1 GB).
        // This test is from the SHA-3 Candidate Algorithm Submissions document [5].
        // The results for SHA-3 are from the Keccak Known Answer Tests [4].
        // The other results are by our own computation.
        auto H = SHA512();
        for (int i = 0; i < 16777216; i++) {
            H.add("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");
        }
        BOOST_CHECK_EQUAL(H.finish(), BigInt<512>("0xb47c933421ea2db1 49ad6e10fce6c7f9 3d0752380180ffd7 f4629a712134831d 77be6091b819ed35 2c2967a2e2d4fa50 50723c9630691f1a 05a7281dbe6c1086"));
    }
}

