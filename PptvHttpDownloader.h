// PptvHttpDownloader.h

#ifndef      _PPBOX_DOWNLOAD_HTTPDOWNLOADER_
#define      _PPBOX_DOWNLOAD_HTTPDOWNLOADER_

#include "ppbox/download/DownloadStatistic.h"

#include <string>
#include <fstream>

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

#include <framework/network/NetName.h>
#include <framework/string/Url.h>
#include <util/protocol/http/HttpRequest.h>

namespace util
{
    namespace protocol
    {
        class HttpClient;
    }
}

namespace ppbox
{
    namespace download
    {
        enum DownloadState
        {
            NOTSTART,
            START,
            COMPLETE,
            UNCOMPLETE,
            CANCEL,
            FAILED,
        };

        struct Range
        {
            boost::uint64_t begin;
            boost::uint64_t end;

            Range()
                : begin(0)
                , end(-1)
            {}

            void reset(void)
            {
                begin = 0;
                end   = -1;
            }
        };

        class PptvHttpDownloader : public PptvDownloadStatistic
        {
        public:
            PptvHttpDownloader(boost::asio::io_service & io_srv,
                               std::ofstream & ofs);

            ~PptvHttpDownloader();

            void SetRange(boost::uint64_t begin, boost::uint64_t end);

            void reset(void);

            DownloadState GetState(void) const;

            typedef boost::function<void (
                boost::system::error_code const &)> open_download_callback_type;

            void async_open(
                util::protocol::HttpRequest const & request,
                framework::network::NetName const & addr,
                open_download_callback_type const & resp);

            void close(void);

            void cancel(void);

        private:
            void http_open_callback(boost::system::error_code const & ec);

            void download_handler(const boost::system::error_code& error,
                                  std::size_t bytes_transferred);

        private:
            util::protocol::HttpClient * http_client_;
            bool                        cancel_;
            util::protocol::HttpRequest request_;
            framework::network::NetName addr_;
            Range range_;
            std::ofstream & ofs_;
            boost::array<char, 1024> buf_;
            boost::uint64_t download_size_;
            boost::uint64_t receive_sum_size_;
            boost::uint64_t leave_size_;
            DownloadState state_;
            boost::system::error_code last_error_;
            open_download_callback_type        resp_;
        };
    }
}

#endif  /* _PPBOX_DOWNLOAD_HTTPDOWNLOADER_ */
