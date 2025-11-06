/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** main
*/

#include <gtest/gtest.h>

int addition(int a, int b) {
    return a + b;
}

TEST(CalculTests, AdditionSimple) {
    EXPECT_EQ(addition(2, 3), 5);
    EXPECT_EQ(addition(-1, 1), 0);
    EXPECT_EQ(addition(0, 0), 0);
}
