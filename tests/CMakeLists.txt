include(GoogleTest)

add_executable(test_schema_infer test_schema_infer.cpp)
target_link_libraries(test_schema_infer PRIVATE scrape_llm_lib GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_schema_infer)

add_executable(test_main_text test_main_text.cpp)
target_link_libraries(test_main_text PRIVATE scrape_llm_lib GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_main_text)

add_executable(test_validator_repair test_validator_repair.cpp)
target_link_libraries(test_validator_repair PRIVATE scrape_llm_lib GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_validator_repair)
