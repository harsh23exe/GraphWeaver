# tests/CMakeLists.txt - Test Configuration

include(GoogleTest)

# Helper function to create tests
function(add_doc_scraper_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        doc_scraper_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
    )
    gtest_discover_tests(${test_name})
endfunction()

# Unit tests - will be added as we implement each module
add_doc_scraper_test(test_models)
add_doc_scraper_test(test_config)
add_doc_scraper_test(test_normalizer)
add_doc_scraper_test(test_utils)
add_doc_scraper_test(test_priority_queue)
add_doc_scraper_test(test_rocksdb_store)
add_doc_scraper_test(test_rate_limiter)
add_doc_scraper_test(test_robots)
add_doc_scraper_test(test_fetcher)
add_doc_scraper_test(test_html_parser)
add_doc_scraper_test(test_detector)
add_doc_scraper_test(test_readability)
add_doc_scraper_test(test_markdown_converter)
add_doc_scraper_test(test_tokenizer)
add_doc_scraper_test(test_chunker)
add_doc_scraper_test(test_content_processor)
add_doc_scraper_test(test_sitemap_parser)
add_doc_scraper_test(test_crawler_integration)
# add_doc_scraper_test(test_fetcher)
# add_doc_scraper_test(test_rate_limiter)
# add_doc_scraper_test(test_html_parser)
# add_doc_scraper_test(test_detector)
# add_doc_scraper_test(test_markdown_converter)
# add_doc_scraper_test(test_chunker)
# add_doc_scraper_test(test_rocksdb_store)

# Placeholder test to verify test infrastructure works
add_executable(test_placeholder test_placeholder.cpp)
target_link_libraries(test_placeholder PRIVATE
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(test_placeholder)
