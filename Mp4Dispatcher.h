// Mp4Dispatcher.h
#ifndef _PPBOX_MP4_DISPATCHER_H_
#define _PPBOX_MP4_DISPATCHER_H_

#include "ppbox/vod/BigMp4.h"

#include <boost/asio/deadline_timer.hpp>

#include <fstream>


namespace ppbox
{
    namespace download
    {

        struct FileDownloadStatistic;
        //∑÷≈‰Sink
        class Mp4Dispatcher
        {
        public:

            Mp4Dispatcher(util::daemon::Daemon & daemon);
            virtual ~Mp4Dispatcher();
            
            boost::system::error_code add(
                std::string const & play_link
                ,std::string const & format
                ,std::string const & dst
                ,std::string  const & filename);

            boost::system::error_code del(boost::system::error_code & ec);

            boost::system::error_code get_download_statictis(
                FileDownloadStatistic & download_statistic,
                boost::system::error_code & ec);

        private:
            void async_open_callback(boost::system::error_code const & ec);
            void async_body_callback(boost::system::error_code const & ec);

            void handle_timer( boost::system::error_code const & ec);

            std::string parse_url(std::string const &url,boost::system::error_code& ec);


        private:
            ppbox::vod::BigMp4 bigmp4_;
            std::string filename_;

            boost::uint32_t head_size_;

            std::fstream file_;
            FileDownloadStatistic* status_;
            boost::asio::deadline_timer timer_;

        };
    }
}

#endif /* _PPBOX_MP4_DISPATCHER_H_ */
