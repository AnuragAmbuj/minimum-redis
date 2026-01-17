//
// Created by Sisyphus on 16/01/26.
//

#include <gtest/gtest.h>
#include "db.h"

// Test fixture for database tests
class DatabaseTest : public ::testing::Test {
protected:
    Database db;

    void SetUp() override {
        // Each test uses a fresh database instance
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// Test String Operations
TEST_F(DatabaseTest, SetGet_Basic) {
    std::string key = "test_key";
    std::string value = "test_value";

    bool result = db.set(key, value);
    EXPECT_TRUE(result);

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, value);

    EXPECT_TRUE(db.exists(key));
    EXPECT_EQ(db.type(key), RedisType::String);
}

TEST_F(DatabaseTest, SetGet_EmptyValue) {
    std::string key = "empty_key";
    std::string value = "";

    bool result = db.set(key, value);
    EXPECT_TRUE(result);

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, value);
}

TEST_F(DatabaseTest, SetGet_SpecialCharacters) {
    std::string key = "special_key";
    std::string value = "hello\nworld\twith\ttabs\r\nand newlines";

    bool result = db.set(key, value);
    EXPECT_TRUE(result);

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, value);
}

TEST_F(DatabaseTest, SetGet_LargeValue) {
    std::string key = "large_key";
    std::string value(10000, 'x');  // 10KB string

    bool result = db.set(key, value);
    EXPECT_TRUE(result);

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, value);
}

TEST_F(DatabaseTest, Get_NonExistentKey) {
    std::string key = "nonexistent";

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, "");

    EXPECT_FALSE(db.exists(key));
}

TEST_F(DatabaseTest, Del_Basic) {
    std::string key = "delete_key";
    std::string value = "delete_value";

    db.set(key, value);
    EXPECT_TRUE(db.exists(key));

    bool result = db.del(key);
    EXPECT_TRUE(result);
    EXPECT_FALSE(db.exists(key));

    std::string retrieved = db.get(key);
    EXPECT_EQ(retrieved, "");
}

TEST_F(DatabaseTest, Del_NonExistentKey) {
    std::string key = "nonexistent";

    bool result = db.del(key);
    EXPECT_FALSE(result);
}

TEST_F(DatabaseTest, Exists_Basic) {
    std::string key = "exists_key";

    EXPECT_FALSE(db.exists(key));

    db.set(key, "value");
    EXPECT_TRUE(db.exists(key));

    db.del(key);
    EXPECT_FALSE(db.exists(key));
}

TEST_F(DatabaseTest, Keys_Empty) {
    std::vector<std::string> keys = db.keys();
    EXPECT_TRUE(keys.empty());
}

TEST_F(DatabaseTest, Keys_Single) {
    std::string key = "single_key";
    db.set(key, "value");

    std::vector<std::string> keys = db.keys();
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], key);
}

TEST_F(DatabaseTest, Keys_Multiple) {
    db.set("key1", "value1");
    db.set("key2", "value2");
    db.set("key3", "value3");

    std::vector<std::string> keys = db.keys();
    EXPECT_EQ(keys.size(), 3);

    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys[0], "key1");
    EXPECT_EQ(keys[1], "key2");
    EXPECT_EQ(keys[2], "key3");
}

TEST_F(DatabaseTest, Type_String) {
    std::string key = "string_key";
    db.set(key, "value");

    EXPECT_EQ(db.type(key), RedisType::String);
}

TEST_F(DatabaseTest, Type_NonExistent) {
    std::string key = "nonexistent";

    EXPECT_EQ(db.type(key), RedisType::String);  // Default type
}

// Test List Operations
TEST_F(DatabaseTest, LPush_Basic) {
    std::string key = "list_key";
    std::vector<std::string> values = {"item1", "item2", "item3"};

    size_t new_length = db.lpush(key, values);
    EXPECT_EQ(new_length, 3);

    EXPECT_EQ(db.type(key), RedisType::List);
    EXPECT_TRUE(db.exists(key));
}

TEST_F(DatabaseTest, LPush_MultipleCalls) {
    std::string key = "list_key";

    db.lpush(key, {"item1", "item2"});
    EXPECT_EQ(db.llen(key), 2);

    db.lpush(key, {"item3"});
    EXPECT_EQ(db.llen(key), 3);
}

TEST_F(DatabaseTest, RPush_Basic) {
    std::string key = "list_key";
    std::vector<std::string> values = {"item1", "item2", "item3"};

    size_t new_length = db.rpush(key, values);
    EXPECT_EQ(new_length, 3);

    EXPECT_EQ(db.type(key), RedisType::List);
}

TEST_F(DatabaseTest, LPop_Basic) {
    std::string key = "list_key";
    db.lpush(key, {"item1", "item2", "item3"});

    std::string popped = db.lpop(key);
    EXPECT_EQ(popped, "item3");  // LPUSH adds to front, so LPOP gets last added
    EXPECT_EQ(db.llen(key), 2);
}

TEST_F(DatabaseTest, LPop_EmptyList) {
    std::string key = "empty_list";

    std::string popped = db.lpop(key);
    EXPECT_EQ(popped, "");
}

TEST_F(DatabaseTest, RPop_Basic) {
    std::string key = "list_key";
    db.rpush(key, {"item1", "item2", "item3"});

    std::string popped = db.rpop(key);
    EXPECT_EQ(popped, "item3");
    EXPECT_EQ(db.llen(key), 2);
}

TEST_F(DatabaseTest, RPop_EmptyList) {
    std::string key = "empty_list";

    std::string popped = db.rpop(key);
    EXPECT_EQ(popped, "");
}

TEST_F(DatabaseTest, LLen_EmptyList) {
    std::string key = "empty_list";

    size_t length = db.llen(key);
    EXPECT_EQ(length, 0);
}

TEST_F(DatabaseTest, LLen_WithItems) {
    std::string key = "list_key";
    db.lpush(key, {"item1", "item2", "item3"});

    size_t length = db.llen(key);
    EXPECT_EQ(length, 3);
}

TEST_F(DatabaseTest, LRange_Basic) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2", "item3", "item4"});

    std::vector<std::string> range = db.lrange(key, 1, 3);
    EXPECT_EQ(range.size(), 3);
    EXPECT_EQ(range[0], "item1");
    EXPECT_EQ(range[1], "item2");
    EXPECT_EQ(range[2], "item3");
}

TEST_F(DatabaseTest, LRange_OutOfBounds) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::vector<std::string> range = db.lrange(key, 1, 10);
    EXPECT_EQ(range.size(), 2);
    EXPECT_EQ(range[0], "item1");
    EXPECT_EQ(range[1], "item2");
}

TEST_F(DatabaseTest, LRange_NegativeIndices) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::vector<std::string> range = db.lrange(key, -2, -1);
    EXPECT_EQ(range.size(), 2);
    EXPECT_EQ(range[0], "item1");
    EXPECT_EQ(range[1], "item2");
}

TEST_F(DatabaseTest, LRange_EmptyRange) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::vector<std::string> range = db.lrange(key, 2, 1);
    EXPECT_TRUE(range.empty());
}

TEST_F(DatabaseTest, LIndex_Basic) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::string item = db.lindex(key, 1);
    EXPECT_EQ(item, "item1");
}

TEST_F(DatabaseTest, LIndex_NegativeIndex) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::string item = db.lindex(key, -1);
    EXPECT_EQ(item, "item2");
}

TEST_F(DatabaseTest, LIndex_OutOfBounds) {
    std::string key = "list_key";
    db.rpush(key, {"item0", "item1", "item2"});

    std::string item = db.lindex(key, 10);
    EXPECT_EQ(item, "");
}

TEST_F(DatabaseTest, LIndex_EmptyList) {
    std::string key = "empty_list";

    std::string item = db.lindex(key, 0);
    EXPECT_EQ(item, "");
}

// Test Type Conflicts
TEST_F(DatabaseTest, TypeConflict_StringToList) {
    std::string key = "conflict_key";

    db.set(key, "string_value");
    EXPECT_EQ(db.type(key), RedisType::String);

    db.lpush(key, {"list_item"});
    EXPECT_EQ(db.type(key), RedisType::List);

    std::string string_val = db.get(key);
    EXPECT_EQ(string_val, "");  // String operation on list returns empty
}

TEST_F(DatabaseTest, TypeConflict_ListToString) {
    std::string key = "conflict_key";

    db.lpush(key, {"list_item"});
    EXPECT_EQ(db.type(key), RedisType::List);

    db.set(key, "string_value");
    EXPECT_EQ(db.type(key), RedisType::String);

    size_t list_len = db.llen(key);
    EXPECT_EQ(list_len, 0);  // List operation on string returns 0
}

// Test Persistence (basic tests, full persistence tested separately)
TEST_F(DatabaseTest, Persistence_Basic) {
    std::string key = "persist_key";
    std::string value = "persist_value";

    db.set(key, value);

    EXPECT_TRUE(db.exists(key));
}