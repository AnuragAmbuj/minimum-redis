#include "db.h"
#include <iostream>

int main() {
    Database db;
    std::cout << "Database test starting..." << std::endl;
    
    RedisValue val;
    val.type = RedisType::String;
    val.str_val = "test_value";
    
    db.set("test_key", val);
    auto result = db.get("test_key");
    if (result) {
        std::cout << "GET worked: " << result->str_val << std::endl;
    }
    
    db.save_rdb("test.rdb");
    std::cout << "SAVE completed" << std::endl;
    
    return 0;
}
