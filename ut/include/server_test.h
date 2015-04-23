
#include "framework.h"

class server_test : public test_fixture
{
public:
    TEST_SUITE(server_test);
        TEST(server_test::test_basic);
        TEST(server_test::test_interrupt_shutdown);
    TEST_SUITE_END();

    virtual ~server_test() throw() {}

    void setup() {}
    void teardown() {}

    void test_basic();
    void test_interrupt_shutdown();
};
