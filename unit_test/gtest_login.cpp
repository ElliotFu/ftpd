#include "../user_inf.h"

#include "gtest/gtest.h"

TEST(user_test, login)
{
    User_Inf user;
    User_Inf::read_passwords();

    // 未设置用户名时验证密码
    EXPECT_EQ (-1, user.check_password(""));

    // 用户名不存在
    EXPECT_EQ (-1, user.check_username("fzh"));

    // 用户名正确
    EXPECT_EQ (0, user.check_username("fuzhihao"));

    // 密码错误
    EXPECT_EQ (-1, user.check_password("111111"));

    // 密码正确
    EXPECT_EQ (0, user.check_password("123456"));
}

int main (int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
// g++ ../user_inf.cpp gtest_login.cpp -o test -lgtest -lpthread -lACE -lssl -lcrypto