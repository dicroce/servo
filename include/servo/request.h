
#ifndef __servo_request_h
#define __servo_request_h

#include "cppkit/ck_socket.h"
#include <memory>
#include <thread>
#include <functional>

namespace servo
{

class request
{
public:
    CK_API request( std::function<void(std::shared_ptr<cppkit::ck_socket> clientSocket )> requestCB ) :
        _clientSocket( std::make_shared<cppkit::ck_socket>() ),
        _thread(),
        _started( false ),
        _done( false ),
        _requestCB( requestCB )
    {
    }

    CK_API virtual ~request() noexcept
    {
        // invalidate client socket...
        _clientSocket->close();

        if( _started )
            _thread.join();
    }

    CK_API inline std::shared_ptr<cppkit::ck_socket>& get_client_socket() { return _clientSocket; }

    CK_API void start() { _thread = std::thread( &request::_thread_entry, this ); }

    CK_API bool is_done() const { return _done; }

protected:
    void _thread_entry() { _started = true; _requestCB( _clientSocket ); _done = true; }

    std::shared_ptr<cppkit::ck_socket> _clientSocket;
    std::thread _thread;
    bool _started;
    bool _running;
    bool _done;
    std::function<void(std::shared_ptr<cppkit::ck_socket> clientSocket)> _requestCB;

private:
    request( const request& ) = delete;
    request& operator = ( const request& ) = delete;
};

}

#endif
