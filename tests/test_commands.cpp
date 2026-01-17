//
// Created by Sisyphus on 16/01/26.
//

#include <gtest/gtest.h>
#include "commands.h"

// Test fixture for command tests
class CommandTest : public ::testing::Test {
protected:
    Database db;
    CommandProcessor processor{db};

    void SetUp() override {
        // Setup fresh database for each test
    }

    void TearDown() override {
        // Cleanup if needed
    }

    // Helper to create RESP array from command parts
    std::shared_ptr<RespValue> createCommandArray(const std::vector<std::string>& parts) {
        auto array = std::make_shared<RespValue>(RespType::Array);
        for (const auto& part : parts) {
            auto bulk_string = std::make_shared<RespValue>(RespType::BulkString);
            bulk_string->str_val = part;
            array->array_val.push_back(bulk_string);
        }
        return array;
    }
};

// Test String Commands
TEST_F(CommandTest, SET_GET_Basic) {
    auto set_cmd = createCommandArray({"SET", "key1", "value1"});
    std::string set_response = processor.process_command(set_cmd);
    EXPECT_EQ(set_response, "+OK\r\n");

    auto get_cmd = createCommandArray({"GET", "key1"});
    std::string get_response = processor.process_command(get_cmd);
    EXPECT_EQ(get_response, "$6\r\nvalue1\r\n");
}

TEST_F(CommandTest, SET_GET_NonExistent) {
    auto get_cmd = createCommandArray({"GET", "nonexistent"});
    std::string response = processor.process_command(get_cmd);
    EXPECT_EQ(response, "$-1\r\n");
}

TEST_F(CommandTest, DEL_Basic) {
    auto set_cmd = createCommandArray({"SET", "key1", "value1"});
    processor.process_command(set_cmd);

    auto del_cmd = createCommandArray({"DEL", "key1"});
    std::string response = processor.process_command(del_cmd);
    EXPECT_EQ(response, ":1\r\n");

    auto get_cmd = createCommandArray({"GET", "key1"});
    std::string get_response = processor.process_command(get_cmd);
    EXPECT_EQ(get_response, "$-1\r\n");
}

TEST_F(CommandTest, EXISTS_Basic) {
    auto exists_cmd1 = createCommandArray({"EXISTS", "nonexistent"});
    std::string response1 = processor.process_command(exists_cmd1);
    EXPECT_EQ(response1, ":0\r\n");

    auto set_cmd = createCommandArray({"SET", "key1", "value1"});
    processor.process_command(set_cmd);

    auto exists_cmd2 = createCommandArray({"EXISTS", "key1"});
    std::string response2 = processor.process_command(exists_cmd2);
    EXPECT_EQ(response2, ":1\r\n");
}

TEST_F(CommandTest, KEYS_Empty) {
    auto keys_cmd = createCommandArray({"KEYS"});
    std::string response = processor.process_command(keys_cmd);
    EXPECT_EQ(response, "*0\r\n");
}

TEST_F(CommandTest, KEYS_WithKeys) {
    auto set1 = createCommandArray({"SET", "key1", "val1"});
    auto set2 = createCommandArray({"SET", "key2", "val2"});
    processor.process_command(set1);
    processor.process_command(set2);

    auto keys_cmd = createCommandArray({"KEYS"});
    std::string response = processor.process_command(keys_cmd);
    // Response should contain both keys in some order
    EXPECT_TRUE(response.find("$4\r\nkey1\r\n") != std::string::npos ||
                response.find("$4\r\nkey2\r\n") != std::string::npos);
}

TEST_F(CommandTest, TYPE_String) {
    auto set_cmd = createCommandArray({"SET", "key1", "value1"});
    processor.process_command(set_cmd);

    auto type_cmd = createCommandArray({"TYPE", "key1"});
    std::string response = processor.process_command(type_cmd);
    EXPECT_EQ(response, "+string\r\n");
}

TEST_F(CommandTest, TYPE_NonExistent) {
    auto type_cmd = createCommandArray({"TYPE", "nonexistent"});
    std::string response = processor.process_command(type_cmd);
    EXPECT_EQ(response, "+none\r\n");
}

// Test List Commands
TEST_F(CommandTest, LPUSH_RPOP_Basic) {
    auto lpush_cmd = createCommandArray({"LPUSH", "list1", "item1", "item2", "item3"});
    std::string push_response = processor.process_command(lpush_cmd);
    EXPECT_EQ(push_response, ":3\r\n");

    auto rpop_cmd = createCommandArray({"RPOP", "list1"});
    std::string pop_response = processor.process_command(rpop_cmd);
    EXPECT_EQ(pop_response, "$5\r\nitem1\r\n");
}

TEST_F(CommandTest, RPUSH_LPOP_Basic) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "item1", "item2", "item3"});
    std::string push_response = processor.process_command(rpush_cmd);
    EXPECT_EQ(push_response, ":3\r\n");

    auto lpop_cmd = createCommandArray({"LPOP", "list1"});
    std::string pop_response = processor.process_command(lpop_cmd);
    EXPECT_EQ(pop_response, "$5\r\nitem1\r\n");
}

TEST_F(CommandTest, LLEN_Basic) {
    auto lpush_cmd = createCommandArray({"LPUSH", "list1", "item1", "item2"});
    processor.process_command(lpush_cmd);

    auto llen_cmd = createCommandArray({"LLEN", "list1"});
    std::string response = processor.process_command(llen_cmd);
    EXPECT_EQ(response, ":2\r\n");
}

TEST_F(CommandTest, LLEN_Empty) {
    auto llen_cmd = createCommandArray({"LLEN", "emptylist"});
    std::string response = processor.process_command(llen_cmd);
    EXPECT_EQ(response, ":0\r\n");
}

TEST_F(CommandTest, LRANGE_Basic) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "one", "two", "three", "four"});
    processor.process_command(rpush_cmd);

    auto lrange_cmd = createCommandArray({"LRANGE", "list1", "1", "3"});
    std::string response = processor.process_command(lrange_cmd);
    EXPECT_EQ(response, "*3\r\n$3\r\ntwo\r\n$5\r\nthree\r\n$4\r\nfour\r\n");
}

TEST_F(CommandTest, LRANGE_OutOfBounds) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "one", "two"});
    processor.process_command(rpush_cmd);

    auto lrange_cmd = createCommandArray({"LRANGE", "list1", "0", "10"});
    std::string response = processor.process_command(lrange_cmd);
    EXPECT_EQ(response, "*2\r\n$3\r\none\r\n$3\r\ntwo\r\n");
}

TEST_F(CommandTest, LINDEX_Basic) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "one", "two", "three"});
    processor.process_command(rpush_cmd);

    auto lindex_cmd = createCommandArray({"LINDEX", "list1", "1"});
    std::string response = processor.process_command(lindex_cmd);
    EXPECT_EQ(response, "$3\r\ntwo\r\n");
}

TEST_F(CommandTest, LINDEX_Negative) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "one", "two", "three"});
    processor.process_command(rpush_cmd);

    auto lindex_cmd = createCommandArray({"LINDEX", "list1", "-1"});
    std::string response = processor.process_command(lindex_cmd);
    EXPECT_EQ(response, "$5\r\nthree\r\n");
}

TEST_F(CommandTest, LINDEX_OutOfBounds) {
    auto rpush_cmd = createCommandArray({"RPUSH", "list1", "one", "two"});
    processor.process_command(rpush_cmd);

    auto lindex_cmd = createCommandArray({"LINDEX", "list1", "10"});
    std::string response = processor.process_command(lindex_cmd);
    EXPECT_EQ(response, "$-1\r\n");
}

// Test Error Conditions
TEST_F(CommandTest, InvalidCommand) {
    auto invalid_cmd = createCommandArray({"INVALID", "arg1"});
    std::string response = processor.process_command(invalid_cmd);
    EXPECT_EQ(response, "-ERR Unknown command\r\n");
}

TEST_F(CommandTest, WrongNumberOfArguments) {
    auto set_cmd = createCommandArray({"SET", "key1"});  // Missing value
    std::string response = processor.process_command(set_cmd);
    EXPECT_EQ(response, "-ERR SET requires exactly 2 arguments\r\n");
}

TEST_F(CommandTest, InvalidArrayFormat) {
    auto invalid_array = std::make_shared<RespValue>(RespType::SimpleString);
    invalid_array->str_val = "not an array";
    std::string response = processor.process_command(invalid_array);
    EXPECT_EQ(response, "-ERR Invalid command format\r\n");
}

TEST_F(CommandTest, InvalidCommandNameType) {
    auto invalid_cmd = createCommandArray({});
    invalid_cmd->array_val.push_back(std::make_shared<RespValue>(RespType::Integer));
    std::string response = processor.process_command(invalid_cmd);
    EXPECT_EQ(response, "-ERR Command name must be a bulk string\r\n");
}

TEST_F(CommandTest, SAVE_Command) {
    auto save_cmd = createCommandArray({"SAVE"});
    std::string response = processor.process_command(save_cmd);
    EXPECT_EQ(response, "+OK\r\n");
}

// Test Type Conflicts
TEST_F(CommandTest, TypeConflict_StringToList) {
    auto set_cmd = createCommandArray({"SET", "key1", "value1"});
    processor.process_command(set_cmd);

    auto type1_cmd = createCommandArray({"TYPE", "key1"});
    std::string type1_response = processor.process_command(type1_cmd);
    EXPECT_EQ(type1_response, "+string\r\n");

    auto lpush_cmd = createCommandArray({"LPUSH", "key1", "listitem"});
    processor.process_command(lpush_cmd);

    auto type2_cmd = createCommandArray({"TYPE", "key1"});
    std::string type2_response = processor.process_command(type2_cmd);
    EXPECT_EQ(type2_response, "+list\r\n");

    auto get_cmd = createCommandArray({"GET", "key1"});
    std::string get_response = processor.process_command(get_cmd);
    EXPECT_EQ(get_response, "$-1\r\n");
}

// Test Command Case Insensitivity
TEST_F(CommandTest, CaseInsensitiveCommands) {
    auto set_cmd = createCommandArray({"set", "key1", "value1"});
    std::string set_response = processor.process_command(set_cmd);
    EXPECT_EQ(set_response, "+OK\r\n");

    auto get_cmd = createCommandArray({"GET", "key1"});  // Mixed case
    std::string get_response = processor.process_command(get_cmd);
    EXPECT_EQ(get_response, "$6\r\nvalue1\r\n");
}

// Test Boundary Conditions
TEST_F(CommandTest, EmptyKey) {
    auto set_cmd = createCommandArray({"SET", "", "value1"});
    std::string response = processor.process_command(set_cmd);
    EXPECT_EQ(response, "+OK\r\n");

    auto get_cmd = createCommandArray({"GET", ""});
    std::string get_response = processor.process_command(get_cmd);
    EXPECT_EQ(get_response, "$6\r\nvalue1\r\n");
}

TEST_F(CommandTest, LargeValues) {
    std::string large_value(10000, 'x');
    auto set_cmd = createCommandArray({"SET", "large_key", large_value});
    std::string response = processor.process_command(set_cmd);
    EXPECT_EQ(response, "+OK\r\n");

    auto get_cmd = createCommandArray({"GET", "large_key"});
    std::string get_response = processor.process_command(get_cmd);
    std::string expected = "$10000\r\n" + large_value + "\r\n";
    EXPECT_EQ(get_response, expected);
}

TEST_F(CommandTest, SpecialCharacters) {
    std::string special_value = "hello\nworld\r\ntab\tand\ttabs\f\n";
    auto set_cmd = createCommandArray({"SET", "special_key", special_value});
    processor.process_command(set_cmd);

    auto get_cmd = createCommandArray({"GET", "special_key"});
    std::string response = processor.process_command(get_cmd);
    std::string expected = "$" + std::to_string(special_value.length()) + "\r\n" + special_value + "\r\n";
    EXPECT_EQ(response, expected);
}