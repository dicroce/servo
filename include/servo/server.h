
#ifndef __servo_server_h
#define __servo_server_h

#include "cppkit/ck_socket.h"
#include "cppkit/ck_socket_address.h"
#include "cppkit/ck_string.h"
#include <mutex>

template<class T>
class server
{
public:
    CK_API server( int port,
                   cppkit::ck_string sockAddr = cppkit::ck_string(),
                   int32_t spinMillis = 200,
                   std::function<void()> loopCB = nullptr ) :
        _serverSocket(),
        _port( port ),
        _sockAddr( sockAddr ),
        _running( false ),
        _activeRequests(),
        _lok(),
        _loopCB( loopCB ),
        _spinMillis( spinMillis )
    {
    }

    CK_API virtual ~server() noexcept
    {
    }

    template<typename... Args>
    CK_API void start( Args&&... args )
    {
        try
        {
            _configure_server_socket();
        }
        catch( std::exception& ex )
        {
            CK_LOG_NOTICE("Exception (%s) caught while initializing server. Exiting.", ex.what());
            return;
        }
        catch( ... )
        {
            CK_LOG_NOTICE("Unknown exception caught while initializing server. Exiting.");
            return;
        }

        _running = true;

        while( _running )
        {
            try
            {
                int timeout = _spinMillis;
                if( !_serverSocket.wait_recv( timeout ) )
                {
                    std::shared_ptr<T> req = std::make_shared<T>( std::forward<Args>(args)... );

                    std::shared_ptr<cppkit::ck_socket>& clientSocket = req->get_client_socket();

                    clientSocket = _serverSocket.accept();

                    clientSocket->linger( 0 );

                    if( !_running )
                        continue;

                    if( clientSocket->buffered_recv() )
                    {
                        std::unique_lock<std::recursive_mutex> g( _lok );

                        _activeRequests.push_back( req );

                        req->start();

                        // while we have the lock held, look for any done requests and clean them up.
                        _activeRequests.remove_if( []( const std::shared_ptr<T>& request )->bool {
                                return request->is_done();
                        } );
                    }
                }
                else
                {
                    try
                    {
                        if( _loopCB )
                            _loopCB();
                    }
                    catch( std::exception& ex )
                    {
                        CK_LOG_NOTICE("Exception (%s) occured in main loop callback.",ex.what());
                    }
                    catch( ... )
                    {
                        CK_LOG_NOTICE("An unknown exception has occurred in main loop callback.");
                    }
                }
            }
            catch( std::exception& ex )
            {
                CK_LOG_NOTICE("Exception (%s) occured while reading request.",ex.what());
            }
            catch( ... )
            {
                CK_LOG_NOTICE("An unknown exception has occurred wile reading request.");
            }
        }
    }

    CK_API void stop()
    {
        _running = false;
    }

private:
    server( const server& ) = delete;
    server& operator = ( const server& ) = delete;

    void _configure_server_socket()
    {
        if( _sockAddr.empty() )
            _serverSocket.bind( _port );
        else _serverSocket.bind( _port, _sockAddr );

        _serverSocket.listen();
    }

    cppkit::ck_socket _serverSocket;
    int _port;
    cppkit::ck_string _sockAddr;
    bool _running;
    std::list<std::shared_ptr<T> > _activeRequests;
    std::recursive_mutex _lok;
    std::function<void()> _loopCB;
    int32_t _spinMillis;
};

#endif
