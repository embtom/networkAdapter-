#include <gtest/gtest.h>
#include <iostream>
#include <cstring>
#include <tuple>
#include <algorithm>
#include <Tcp/TcpDataLink.hpp>

#define GTEST_BOX                   "[     cout ] "

using namespace EtNet;

TEST(DataLink, MoveKonstruktor)
{
 //   auto [a] = std::tuple(CTcpDataLink(3));
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}