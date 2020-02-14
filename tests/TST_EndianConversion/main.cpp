#include <gtest/gtest.h>
#include <iostream>
#include <detail/EndianConvert.h>
#include <detail/EndianMembers.h>
#include <detail/EndianMeta.h>
#include <NetOrder.h>
#include <HostOrder.h>


#include "SwapEndian.h"

#define GTEST_BOX                   "[     cout ] "


struct STestStructure
{
   public:
   bool operator==(const STestStructure& rhs) const
   {
      if ((member16 == rhs.member16) &&
          (member32 == rhs.member32) &&
          (arrayOf16 == rhs.arrayOf16) &&
          (arrayOf32 == rhs.arrayOf32))
      {
         return true;
      }
      return false;
   }

   bool operator!=(const STestStructure& rhs) const
   {
      return !(*this == rhs);
   }


   uint16_t                member16;
   uint32_t                member32;
   std::array<uint16_t,4>  arrayOf16;
   std::array<uint32_t,8>  arrayOf32;
};


namespace EtEndian
{

template <>
inline auto registerName<STestStructure>()
{
   return "STestStructure";
}

template <>
inline auto registerMembers<STestStructure>()
{
   return members(
      member("member16",   &STestStructure::member16),
      member("member32",   &STestStructure::member32),
      member("arrayOf16",  &STestStructure::arrayOf16),
      member("arrayOf32",  &STestStructure::arrayOf32)
   );
}
}

TEST(EndianConvert, ConvertByteOrder)
{
    {
        std::cout << std::hex;

        uint16_t a = 5;
        uint16_t b = EtEndian::host_to_network(a);
        uint16_t verify = EtTest::swapEndian(a);
        std::cout << GTEST_BOX << "A: " << a << " B: " << b << std::endl;

        uint16_t c = EtEndian::network_to_host(b);
        std::cout << GTEST_BOX << "B: " << b << " C: " << c << std::endl;
        EXPECT_EQ(b,verify);
        EXPECT_EQ(a,c);
    }
    {
        std::cout << std::hex;

        uint32_t a = 0x01020304;
        uint32_t b = EtEndian::host_to_network(a);
        std::cout << GTEST_BOX << "A: " << a << " B: " << b << std::endl;
        uint32_t verify = EtTest::swapEndian(a);

        uint32_t c = EtEndian::network_to_host(b);
        std::cout << GTEST_BOX << "B: " << b << " C: " << c << std::endl;
        EXPECT_EQ(b,verify);
        EXPECT_EQ(a,c);
    }
    {
        std::cout << std::hex;

        uint64_t a = 0x01020304;
        uint64_t b = EtEndian::host_to_network(a);
        std::cout << GTEST_BOX << "A: " << a << " B: " << b << std::endl;
        uint64_t verify = EtTest::swapEndian(a);

        uint32_t c = EtEndian::network_to_host(b);
        std::cout << GTEST_BOX << "B: " << b << " C: " << c << std::endl;
        EXPECT_EQ(b,verify);
        EXPECT_EQ(a,c);
    }

}

TEST(EndianMeta, EndianMembers)
{
   STestStructure person({15, 39, {1,2,3,4}});
   {
      auto bla = EtEndian::member("arrayOf16",  &STestStructure::member32);
      std::cout << GTEST_BOX << "VarName: " << bla.getName() << std::endl;
      using MemberT = EtEndian::get_member_type<decltype(bla)>;
      const MemberT test = bla.get(person);
      bla.set(person,120);
   }
   {
      auto bla = EtEndian::member("arrayOf16",  &STestStructure::arrayOf16);
      std::cout << GTEST_BOX << "VarName: " << bla.getName() << std::endl;
      using MemberT = EtEndian::get_member_type<decltype(bla)>;
      MemberT test = bla.get(person);
      MemberT test1 {1,2,3,4};
   }
   {
      auto bla = EtEndian::member("arrayOf16",  &STestStructure::arrayOf16);
      using MemberT = EtEndian::get_member_type<decltype(bla)>;
      MemberT& test = bla.getRef(person);
      test[0] = 500;
   }
}

TEST(EndianMeta, EndianMeta)
{
   EXPECT_EQ(EtEndian::getMemberCount<STestStructure>(), 4);
   EXPECT_EQ(EtEndian::getName<STestStructure>(), "STestStructure");
   EXPECT_EQ(EtEndian::isRegistered<STestStructure>(), true);

   STestStructure person({15, 39, {1,2,3,4}});

   EtEndian::doForAllMembers<STestStructure>(
      [&person](const auto& member)
   {
      using MemberT = EtEndian::get_member_type<decltype(member)>;
      MemberT td = member.get(person);
      std::cout << GTEST_BOX << member.getName() << std::endl;
   });
}

TEST(NetOrder, ConvertToNet)
{
   STestStructure testDataVerify ({0xfaba, 0x020408ff,
                            {0x1122, 0x3344, 0xAABB, 0xCCDD},
                            {0x1122AABB, 0x3344CCDD, 0xAABBEEFF, 0xCCDD1122, 0x1223ABBC, 0x3445CDDE, 0xABBCEFFA, 0xCDDE1223}});

   STestStructure testDataVerifyNetOrder(
      {EtEndian::host_to_network(testDataVerify.member16), EtEndian::host_to_network(testDataVerify.member32)} );

   //todo should be a constexpr initializer
   std::transform(testDataVerify.arrayOf16.begin(), testDataVerify.arrayOf16.end(), testDataVerifyNetOrder.arrayOf16.begin(),
         [](const auto& a) { return  EtEndian::host_to_network(a); });

   std::transform(testDataVerify.arrayOf32.begin(), testDataVerify.arrayOf32.end(), testDataVerifyNetOrder.arrayOf32.begin(),
         [](const auto& a) { return  EtEndian::host_to_network(a); });


   {
      EtEndian::CNetOrder converter (testDataVerify);
      const STestStructure& TestHostOrder = converter.HostOrder();
      const STestStructure& TestNetOrder = converter.NetworkOrder();

      EXPECT_EQ(TestHostOrder,testDataVerify);
      EXPECT_NE(TestNetOrder,testDataVerify);
      EXPECT_EQ(TestNetOrder,testDataVerifyNetOrder);
   }

   {
      EtEndian::CNetOrder converter (STestStructure({0xfaba, 0x020408ff,
                                                    {0x1122, 0x3344, 0xAABB, 0xCCDD},
                                                    {0x1122AABB, 0x3344CCDD, 0xAABBEEFF, 0xCCDD1122, 0x1223ABBC, 0x3445CDDE, 0xABBCEFFA, 0xCDDE1223}})
                                                   );
      const STestStructure& TestHostOrder = converter.HostOrder();
      const STestStructure& TestNetOrder = converter.NetworkOrder();

      EXPECT_EQ(TestHostOrder,testDataVerify);
      EXPECT_NE(TestNetOrder,testDataVerify);
      EXPECT_EQ(TestNetOrder,testDataVerifyNetOrder);
   }
   {
      EtEndian::CNetOrder converterToNet (testDataVerify);
      const STestStructure& TestHostOrder = converterToNet.HostOrder();
      const STestStructure& TestNetOrder = converterToNet.NetworkOrder();
      EXPECT_EQ(TestHostOrder,testDataVerify);
      EXPECT_EQ(TestNetOrder,testDataVerifyNetOrder);

      EtEndian::CHostOrder converterToHost (TestNetOrder);
      const STestStructure& TestHostOrder2 = converterToHost.HostOrder();
      const STestStructure& TestNetOrder2 = converterToHost.NetworkOrder();
      EXPECT_EQ(TestHostOrder,TestHostOrder2);
      EXPECT_EQ(TestNetOrder,TestNetOrder2);

      EtEndian::CHostOrder converter3(converterToHost);
      const STestStructure& TestHostOrder3 = converter3.HostOrder();
      const STestStructure& TestNetOrder3 = converter3.NetworkOrder();
      EXPECT_EQ(TestHostOrder,TestHostOrder3);
      EXPECT_EQ(TestNetOrder,TestNetOrder3);
   }


}


int main(int argc, char **argv)
{
   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
