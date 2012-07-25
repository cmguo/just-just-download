#ifndef _PPBOX_PUT_RTPSINK_H_
#define _PPBOX_PUT_RTPSINK_H_

#include "ppbox/download/DownloadSink.h"

#include <util/protocol/http/HttpClient.h>
#include <util/protocol/http/HttpChunkedSocket.h>

namespace ppbox
{
    namespace download
    {

        class PutSink 
		    : public DownloadSink
        {
        public:
            PutSink(
                boost::asio::io_service & io_svc, 
                std::string http_file); 

            virtual ~PutSink();

        private:
            boost::system::error_code on_finish(
                boost::system::error_code const &ec);

            virtual boost::system::error_code write(
                boost::posix_time::ptime const & time_send,
                ppbox::demux::Sample&);

        private:
            util::protocol::HttpClient http_;
            util::protocol::HttpChunkedSocket<util::protocol::HttpSocket> socket_;
            std::string http_file_;
            bool bRunning;
        };

    } // namespace download
} // namespace ppbox

#endif 
