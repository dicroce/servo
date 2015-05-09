
#ifndef __servo_server_threaded_h
#define __servo_server_threaded_h

#include "cppkit/ck_socket.h"
#include "cppkit/ck_socket_address.h"
#include "cppkit/ck_string.h"
#include "cppkit/os/ck_platform.h"
#include <mutex>
#include <thread>

namespace servo
{

struct conn_context
{
    bool done;
    std::shared_ptr<cppkit::ck_socket> connected;
    std::thread th;
};

class server_threaded
{
public:
    CK_API server_threaded( int port,
                   std::function<void(std::shared_ptr<cppkit::ck_socket>)> connCB,
                   cppkit::ck_string sockAddr = cppkit::ck_string() ) :
        _serverSocket(),
        _port( port ),
        _connCB( connCB ),
        _sockAddr( sockAddr ),
        _connectedContexts(),
        _running( false )
    {
    }

    CK_API virtual ~server_threaded() noexcept
    {
        for( const std::shared_ptr<conn_context>& c : _connectedContexts )
        {
            c->connected->close();
            c->th.join();
        }
    }

    CK_API void stop()
    {
        _running = false;

        FULL_MEM_BARRIER();

        cppkit::ck_socket sok;
        sok.connect( (_sockAddr.empty()) ? "127.0.0.1" : _sockAddr, _port );
    }

    CK_API void start()
    {
        try
        {
            _configure_server_socket();
        }
        catch( std::exception& ex )
        {
            CK_LOG_NOTICE("Exception (%s) caught while initializing server_threaded. Exiting.", ex.what());
            return;
        }
        catch( ... )
        {
            CK_LOG_NOTICE("Unknown exception caught while initializing server_threaded. Exiting.");
            return;
        }

        _running = true;

        while( _running )
        {
            try
            {
                std::shared_ptr<struct conn_context> cc = std::make_shared<struct conn_context>();

                cc->connected = _serverSocket.accept();

                if( !_running )
                    continue;

                if( cc->connected->buffered_recv() )
                {
                    cc->done = false;
                    cc->th = std::thread( &server_threaded::_thread_start, this, cc );

                    _connectedContexts.remove_if( []( const std::shared_ptr<struct conn_context>& context )->bool {
                        if( context->done )
                        {
                            context->th.join();
                            return true;
                        }
                        return false;
                    } );

                    _connectedContexts.push_back( cc );
                }
            }
            catch( std::exception& ex )
            {
                CK_LOG_NOTICE("Exception (%s) occured while responding to connection.",ex.what());
            }
            catch( ... )
            {
                CK_LOG_NOTICE("An unknown exception has occurred while responding to connection.");
            }
        }
    }

private:
    server_threaded( const server_threaded& ) = delete;
    server_threaded& operator = ( const server_threaded& ) = delete;

    void _configure_server_socket()
    {
        if( _sockAddr.empty() )
            _serverSocket.bind( _port );
        else _serverSocket.bind( _port, _sockAddr );

        _serverSocket.listen();
    }

    void _thread_start( std::shared_ptr<struct conn_context> cc ) { _connCB( cc->connected ); cc->done = true; }

    cppkit::ck_socket _serverSocket;
    int _port;
    std::function<void(std::shared_ptr<cppkit::ck_socket>)> _connCB;
    cppkit::ck_string _sockAddr;
    std::list<std::shared_ptr<struct conn_context> > _connectedContexts;
    bool _running;
};

}

#endif
