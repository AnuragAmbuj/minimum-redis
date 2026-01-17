//
// Created by Sisyphus on 16/01/26.
//

#include <gtest/gtest.h>
#include "resp.h"

// Test fixture for RESP parser tests
class RespParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test Simple Strings
TEST_F(RespParserTest, SimpleString_Basic) {
    const char* data = "+OK\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::SimpleString);
    EXPECT_EQ(result->str_val, "OK");
}

TEST_F(RespParserTest, SimpleString_Empty) {
    const char* data = "+\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::SimpleString);
    EXPECT_EQ(result->str_val, "");
}

TEST_F(RespParserTest, SimpleString_WithSpaces) {
    const char* data = "+hello world\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::SimpleString);
    EXPECT_EQ(result->str_val, "hello world");
}

// Test Errors
TEST_F(RespParserTest, Error_Basic) {
    const char* data = "-ERR something wrong\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Error);
    EXPECT_EQ(result->str_val, "ERR something wrong");
}

TEST_F(RespParserTest, Error_Empty) {
    const char* data = "-\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Error);
    EXPECT_EQ(result->str_val, "");
}

// Test Integers
TEST_F(RespParserTest, Integer_Positive) {
    const char* data = ":123\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Integer);
    EXPECT_EQ(result->int_val, 123LL);
}

TEST_F(RespParserTest, Integer_Negative) {
    const char* data = ":-456\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Integer);
    EXPECT_EQ(result->int_val, -456LL);
}

TEST_F(RespParserTest, Integer_Zero) {
    const char* data = ":0\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Integer);
    EXPECT_EQ(result->int_val, 0LL);
}

TEST_F(RespParserTest, Integer_Large) {
    const char* data = ":9223372036854775807\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Integer);
    EXPECT_EQ(result->int_val, 9223372036854775807LL);
}

// Test Bulk Strings
TEST_F(RespParserTest, BulkString_Basic) {
    const char* data = "$5\r\nhello\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val, "hello");
}

TEST_F(RespParserTest, BulkString_Empty) {
    const char* data = "$0\r\n\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val, "");
}

TEST_F(RespParserTest, BulkString_WithNewlines) {
    const char* data = "$11\r\nhello\nworld\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val, "hello\nworld");
}

TEST_F(RespParserTest, BulkString_Null) {
    const char* data = "$-1\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val, "");
}

// Test Arrays
TEST_F(RespParserTest, Array_Empty) {
    const char* data = "*0\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 0);
}

TEST_F(RespParserTest, Array_SingleElement) {
    const char* data = "*1\r\n$4\r\ntest\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 1);
    EXPECT_EQ(result->array_val[0]->type, RespType::BulkString);
    EXPECT_EQ(result->array_val[0]->str_val, "test");
}

TEST_F(RespParserTest, Array_MultipleElements) {
    const char* data = "*3\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$5\r\nhello\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 3);
    EXPECT_EQ(result->array_val[0]->str_val, "foo");
    EXPECT_EQ(result->array_val[1]->str_val, "bar");
    EXPECT_EQ(result->array_val[2]->str_val, "hello");
}

TEST_F(RespParserTest, Array_MixedTypes) {
    const char* data = "*4\r\n+OK\r\n:-123\r\n$0\r\n\r\n*0\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 4);
    EXPECT_EQ(result->array_val[0]->type, RespType::SimpleString);
    EXPECT_EQ(result->array_val[0]->str_val, "OK");
    EXPECT_EQ(result->array_val[1]->type, RespType::Integer);
    EXPECT_EQ(result->array_val[1]->int_val, -123LL);
    EXPECT_EQ(result->array_val[2]->type, RespType::BulkString);
    EXPECT_EQ(result->array_val[2]->str_val, "");
    EXPECT_EQ(result->array_val[3]->type, RespType::Array);
    EXPECT_EQ(result->array_val[3]->array_val.size(), 0);
}

// Test Error Conditions
TEST_F(RespParserTest, InvalidType) {
    const char* data = "Xinvalid\r\n";
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, IncompleteData) {
    const char* data = "$5\r\nhello";  // Missing \r\n
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, IncompleteBulkString) {
    const char* data = "$5\r\nhel";  // Data shorter than length
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, IncompleteArray) {
    const char* data = "*2\r\n$3\r\nfoo\r\n";  // Missing second element
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, InvalidInteger) {
    const char* data = ":abc\r\n";  // Non-numeric integer
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, InvalidBulkStringLength) {
    const char* data = "$abc\r\nhello\r\n";  // Non-numeric length
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, InvalidArrayLength) {
    const char* data = "*abc\r\n";  // Non-numeric array length
    RespParser parser(data, strlen(data));

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

// Test Boundary Conditions
TEST_F(RespParserTest, EmptyBuffer) {
    const char* data = "";
    RespParser parser(data, 0);

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, BufferTooSmall) {
    const char* data = "+";
    RespParser parser(data, 1);

    EXPECT_THROW({
        parser.parse();
    }, std::runtime_error);
}

TEST_F(RespParserTest, MaximumBulkString) {
    std::string data = "$1000000\r\n";
    data += std::string(1000000, 'x') + "\r\n";
    RespParser parser(data.c_str(), data.length());
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val.length(), 1000000);
}

TEST_F(RespParserTest, ZeroLengthBulkString) {
    const char* data = "$0\r\n\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::BulkString);
    EXPECT_EQ(result->str_val, "");
}

// Test Redis Command Format
TEST_F(RespParserTest, RedisCommand_SET) {
    const char* data = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 3);
    EXPECT_EQ(result->array_val[0]->str_val, "SET");
    EXPECT_EQ(result->array_val[1]->str_val, "key");
    EXPECT_EQ(result->array_val[2]->str_val, "value");
}

TEST_F(RespParserTest, RedisCommand_GET) {
    const char* data = "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
    RespParser parser(data, strlen(data));
    auto result = parser.parse();

    ASSERT_TRUE(result != nullptr);
    EXPECT_EQ(result->type, RespType::Array);
    EXPECT_EQ(result->array_val.size(), 2);
    EXPECT_EQ(result->array_val[0]->str_val, "GET");
    EXPECT_EQ(result->array_val[1]->str_val, "key");
}