
#include "server_test.h"
#include "servo/server.h"
#include "servo/request.h"
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

    thread t( [&]() {
        server<request> s( port );
        s.start( [&]( shared_ptr<ck_socket> connected ) {
            unsigned int val = 0;
            connected->recv( &val, 4 );
            ++val;
            connected->send( &val, 4 );

            s.stop(); // stop the server...
        } );
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
    int done = 0;

    {
        server<request> s( port );

        thread t1( [&]() {
            s.start( [&]( shared_ptr<ck_socket> connected ) {
                while( connected->valid() )
                {
                    ++numLoops;
                    ck_usleep( 10000 );
                }
                ++done;
            } );
        } );

        ck_usleep( 50000 );

        ck_socket clientSocket;
        TRY_N_TIMES( clientSocket.query_connect( "127.0.0.1", port ), 20 );

        unsigned int val = 41;
        clientSocket.send( &val, 4 );

        ck_usleep( 250000 );

        s.stop();

        t1.join();
    }

    UT_ASSERT( numLoops > 1 );
    UT_ASSERT( done == 1 );
}
