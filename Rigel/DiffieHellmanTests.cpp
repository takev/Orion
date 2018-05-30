#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DiffieHellman
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include "DiffieHellman.hpp"

using namespace std;
using namespace boost;
using namespace Orion::Rigel;

BOOST_AUTO_TEST_CASE(TestDoubleUnique)
{
    {
        // example from: https://en.wikipedia.org/wiki/Diffie%E2%80%93Hellman_key_exchange
        auto A = DiffieHellman<64>(BigInt<64>(5), BigInt<64>(23), BigInt<64>(4));
        auto B = DiffieHellman<64>(BigInt<64>(5), BigInt<64>(23), BigInt<64>(3));

        A.setTheirPublicKey(B.myPublicKey);
        B.setTheirPublicKey(A.myPublicKey);
  
        BOOST_CHECK_EQUAL(A.sharedKey, BigInt<64>(18));
        BOOST_CHECK_EQUAL(B.sharedKey, BigInt<64>(18));
        BOOST_CHECK_EQUAL(A.sharedKey, B.sharedKey);
    }

    {
        auto A = DiffieHellman<1536>(
            BigInt<1536>("2"),
            BigInt<1536>("0xffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7edee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3dc2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f83655d23dca3ad961c62f356208552bb9ed529077096966d670c354e4abc9804f1746c08ca237327ffffffffffffffff"),
            BigInt<1536>("0xb3631020eedf5d5202f3887e7c8b6351d9f10ef44128fb372141544caafca06becd1aed969c895b6220c47431ae437ae2f2495de61d46744219f30b45287d609e161a5aa019f54958fdb4548b9475a86646c26ae7004c1b2b675c1cf48d5558a32573cbd5d51db74f3e1852748b59f375ae23b624ee4cce9bb5d67a0bbad14e32b6f6e65bd1599a910c0e73bb5147320a009c61c881bd04350f9aebd68c64e2796597ecd39ab892a6e228846c11b911464cfb85992f583850cfece0227816eae")
        );

        BOOST_CHECK_EQUAL(A.myPublicKey, BigInt<1536>("0x3601909ce0ed1b3a0a2319ff2dc7dfe297203dd56afe0918375d4c1a1b4f311e0ee75262d991279fdc5481fc9cc8b8c927d35fd9316df2429936cfc3c872fee1a8a5cf9d4e07a00b4284556e26c84a64c1064674731a0788d46d2d1de03911f3b3840912ffb96e5fc899190c36561b291de58bef357c2f47df0d7d0871a0e3e7eea7e020e3d40daecba32948a98897967203bd2eaad379757ae05e0feb2caa97e2d4b31e5ff32fd0459c1622ec7204056802123c48af708f7132f8135f56d2fa"));
    }

    for (int i = 0; i < 10; i++) {
        auto A = DiffieHellman<1536>(group_5_g, group_5_m);
        auto B = DiffieHellman<1536>(group_5_g, group_5_m);

        A.setTheirPublicKey(B.myPublicKey);
        B.setTheirPublicKey(A.myPublicKey);

        BOOST_CHECK_EQUAL(A.sharedKey, B.sharedKey);
        BOOST_CHECK_EQUAL(A.getKeyingMaterial(), B.getKeyingMaterial());
    }

    for (int i = 0; i < 2; i++) {
        auto A = DiffieHellman<4096>(group_16_g, group_16_m);
        auto B = DiffieHellman<4096>(group_16_g, group_16_m);

        A.setTheirPublicKey(B.myPublicKey);
        B.setTheirPublicKey(A.myPublicKey);

        BOOST_CHECK_EQUAL(A.sharedKey, B.sharedKey);
        BOOST_CHECK_EQUAL(A.getKeyingMaterial(), B.getKeyingMaterial());
    }
}

