
#include "server_test.h"
#include "servo/server.h"
#include "cppkit/os/ck_time_utils.h"

using namespace std;
using namespace cppkit;
using namespace servo;

REGISTER_TEST_FIXTURE(server_test);

#define TRY_N_TIMES(a,b)     \
{                            \
    int tries = 0;           \
    while(!a && (tries < b)) \
    {                        \
        ck_usleep(100000);   \
        tries++;             \
    }                        \
}

void server_test::test_basic()
{
    int port = UT_NEXT_PORT();

    server s( port, [&s]( shared_ptr<ck_socket> connected ) {
        unsigned int val = 0;
        connected->recv( &val, 4 );
        ++val;
        connected->send( &val, 4 );
        s.stop();
    } );

    thread t( [&]() {
        s.start();
    } );

    unsigned int val = 41;
    ck_socket clientSocket;

    TRY_N_TIMES( clientSocket.query_connect( "127.0.0.1", port ), 20 );

    clientSocket.send( &val, 4 );
    clientSocket.recv( &val, 4 );

    UT_ASSERT( val == 42 );

    t.join();
}

void server_test::test_interrupt_shutdown()
{
    int port = UT_NEXT_PORT();

    int numLoops = 0;

    server s( port, [&]( shared_ptr<ck_socket> connected ) {
        while( connected->valid() )
        {
            ++numLoops;
            ck_usleep( 10000 );
        }
    } );

    thread t( [&]() {
        s.start();
    } );

    ck_usleep( 50000 );

    ck_socket clientSocket;
    TRY_N_TIMES( clientSocket.query_connect( "127.0.0.1", port ), 20 );

    // Only connections with data get serviced, so if we want to check numLoops we have to write
    // something...
    unsigned int val = 41;
    clientSocket.send( &val, 4 );

    ck_usleep( 250000 );

    s.stop();

    t.join();

    UT_ASSERT( numLoops > 0 );
}
