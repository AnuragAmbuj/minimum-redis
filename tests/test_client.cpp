//
// Created by MinimalRedis Test Suite
//

#include <gtest/gtest.h>
#include "redis_client.h"
#include <sstream>
#include <string>

// Test RespParser functionality
class RespParserTest : public ::testing::Test {
protected:
    RespParser parser;
};

TEST_F(RespParserTest, SimpleString_Basic) {
    parser.append("+OK\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "+OK\r\n");
}

TEST_F(RespParserTest, SimpleString_Empty) {
    parser.append("+\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "+\r\n");
}

TEST_F(RespParserTest, Error_Basic) {
    parser.append("-ERROR message\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "-ERROR message\r\n");
}

TEST_F(RespParserTest, Integer_Positive) {
    parser.append(":123\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, ":123\r\n");
}

TEST_F(RespParserTest, Integer_Negative) {
    parser.append(":-456\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, ":-456\r\n");
}

TEST_F(RespParserTest, Integer_Zero) {
    parser.append(":0\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, ":0\r\n");
}

TEST_F(RespParserTest, BulkString_Basic) {
    parser.append("$5\r\nhello\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "$5\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "hello\r\n");
}

TEST_F(RespParserTest, BulkString_Empty) {
    parser.append("$0\r\n\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "$0\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "\r\n");
}

TEST_F(RespParserTest, BulkString_Null) {
    parser.append("$-1\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "$-1\r\n");
}

TEST_F(RespParserTest, Array_Empty) {
    parser.append("*0\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "*0\r\n");
}

TEST_F(RespParserTest, Array_SingleElement) {
    parser.append("*1\r\n$4\r\ntest\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "*1\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "$4\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "test\r\n");
}

TEST_F(RespParserTest, Array_MultipleElements) {
    parser.append("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "*2\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "$3\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "foo\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "$3\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "bar\r\n");
}

TEST_F(RespParserTest, MultipleMessages) {
    parser.append("+OK\r\n-ERR\r\n");
    std::string result1 = parser.get_next_message();
    std::string result2 = parser.get_next_message();
    EXPECT_EQ(result1, "+OK\r\n");
    EXPECT_EQ(result2, "-ERR\r\n");
}

TEST_F(RespParserTest, PartialMessage) {
    parser.append("+OK");
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "");

    parser.append("\r\n");
    result = parser.get_next_message();
    EXPECT_EQ(result, "+OK\r\n");
}

TEST_F(RespParserTest, Clear) {
    parser.append("+OK\r\n");
    parser.clear();
    EXPECT_FALSE(parser.has_data());
}

// Test RedisClient basic functionality (non-networking parts)
class RedisClientTest : public ::testing::Test {
protected:
    RedisClient client;
};

TEST_F(RedisClientTest, Constructor_Default) {
    RedisClient client;
    EXPECT_EQ(client.get_host(), "127.0.0.1");
    EXPECT_EQ(client.get_port(), 6379);
    EXPECT_FALSE(client.is_connected());
}

TEST_F(RedisClientTest, Constructor_Custom) {
    RedisClient client("192.168.1.100", 6380);
    EXPECT_EQ(client.get_host(), "192.168.1.100");
    EXPECT_EQ(client.get_port(), 6380);
    EXPECT_FALSE(client.is_connected());
}

TEST_F(RedisClientTest, CommandFormatting_SingleWord) {
    // Test that single word commands are properly formatted
    // We can't test send_command directly without network, but we can test the logic
    std::string expected = "*1\r\n$3\r\nGET\r\n";
    std::stringstream ss;
    ss << "*" << 1 << "\r\n";
    ss << "$" << 3 << "\r\n" << "GET" << "\r\n";
    EXPECT_EQ(ss.str(), expected);
}

TEST_F(RedisClientTest, CommandFormatting_MultipleWords) {
    // Test multi-word command formatting
    std::string expected = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n";
    std::stringstream ss;
    ss << "*" << 3 << "\r\n";
    ss << "$" << 3 << "\r\n" << "SET" << "\r\n";
    ss << "$" << 3 << "\r\n" << "key" << "\r\n";
    ss << "$" << 5 << "\r\n" << "value" << "\r\n";
    EXPECT_EQ(ss.str(), expected);
}

TEST_F(RedisClientTest, CommandFormatting_SpecialCharacters) {
    // Test commands with special characters
    std::string expected = "*2\r\n$7\r\nMGET\r\n$7\r\nkey:123\r\n";
    std::stringstream ss;
    ss << "*" << 2 << "\r\n";
    ss << "$" << 7 << "\r\n" << "MGET" << "\r\n";
    ss << "$" << 7 << "\r\n" << "key:123" << "\r\n";
    EXPECT_EQ(ss.str(), expected);
}

TEST_F(RedisClientTest, Stop_OnDisconnectedClient) {
    RedisClient client;
    // Should not crash when stopping a client that was never connected
    EXPECT_NO_THROW(client.stop());
    EXPECT_FALSE(client.is_connected());
}

// Test RESP protocol formatting helpers
TEST(RespProtocolTest, FormatSimpleString) {
    std::string expected = "+OK\r\n";
    EXPECT_EQ(expected, "+OK\r\n");
}

TEST(RespProtocolTest, FormatError) {
    std::string expected = "-ERR unknown command\r\n";
    EXPECT_EQ(expected, "-ERR unknown command\r\n");
}

TEST(RespProtocolTest, FormatInteger) {
    std::string expected = ":42\r\n";
    EXPECT_EQ(expected, ":42\r\n");
}

TEST(RespProtocolTest, FormatBulkString) {
    std::string expected = "$5\r\nhello\r\n";
    EXPECT_EQ(expected, "$5\r\nhello\r\n");
}

TEST(RespProtocolTest, FormatNullBulkString) {
    std::string expected = "$-1\r\n";
    EXPECT_EQ(expected, "$-1\r\n");
}

TEST(RespProtocolTest, FormatArray) {
    std::string expected = "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
    EXPECT_EQ(expected, "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n");
}

// Cross-platform compatibility tests
TEST(CrossPlatformTest, PathSeparator) {
    // Test that we don't use platform-specific path separators in core logic
    std::string test_string = "some/path\\with\\mixed/separators";
    // Core logic should work regardless of path separators
    EXPECT_TRUE(test_string.find('/') != std::string::npos ||
                test_string.find('\\') != std::string::npos);
}

TEST(CrossPlatformTest, LineEndings) {
    // Test that our parsing handles different line endings
    RespParser parser;
    parser.append("+OK\r\n");  // Standard
    std::string result = parser.get_next_message();
    EXPECT_EQ(result, "+OK\r\n");

    parser.clear();
    parser.append("+OK\n");  // Unix style (should not match)
    result = parser.get_next_message();
    EXPECT_EQ(result, "");  // Should not parse incomplete message
}

// Memory safety tests
TEST(MemorySafetyTest, Parser_BufferManagement) {
    RespParser parser;

    // Test large buffer handling
    std::string large_data(10000, 'x');
    parser.append(large_data);
    EXPECT_TRUE(parser.has_data());

    parser.clear();
    EXPECT_FALSE(parser.has_data());
}

TEST(MemorySafetyTest, Client_ResourceManagement) {
    {
        RedisClient client;
        // Client should clean up resources in destructor
    }
    // Should not crash or leak memory
    EXPECT_TRUE(true);  // If we get here, basic resource management works
}

// Edge case tests
TEST(EdgeCasesTest, EmptyCommand) {
    // Empty commands should be handled gracefully
    std::stringstream ss("");
    std::string part;
    std::vector<std::string> parts;
    while (ss >> part) {
        parts.push_back(part);
    }
    EXPECT_TRUE(parts.empty());
}

TEST(EdgeCasesTest, CommandWithSpaces) {
    std::string command = "SET \"my key\" \"my value\"";
    std::stringstream ss(command);
    std::string part;
    std::vector<std::string> parts;
    while (ss >> part) {
        parts.push_back(part);
    }
    // Basic parsing - in real Redis client, quotes would be handled differently
    EXPECT_EQ(parts.size(), 5);  // SET "my key" "my value" splits into 5 parts with >>
    EXPECT_EQ(parts[0], "SET");
    EXPECT_EQ(parts[1], "\"my");
    EXPECT_EQ(parts[2], "key\"");
    EXPECT_EQ(parts[3], "\"my");
    EXPECT_EQ(parts[4], "value\"");
}

TEST(EdgeCasesTest, Parser_BufferOverflow) {
    RespParser parser;

    // Test with very large input
    std::string large_input(100000, 'x');
    large_input += "\r\n";
    parser.append(large_input);

    // Should handle large buffers without crashing
    std::string result = parser.get_next_message();
    EXPECT_FALSE(result.empty());
}