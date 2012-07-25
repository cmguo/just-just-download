#ifndef _PPBOX_DOWNLOAD_MUX_DOWNLOADHER_H_
#define _PPBOX_DOWNLOAD_MUX_DOWNLOADHER_H_

#include "ppbox/download/Downloader.h"

namespace ppbox
{
    namespace mux
    {
        class Dispatcher;
        class Sink;
    }

    namespace download
    { 

        class MuxDownloader 
            : public Downloader
        {
        public:
            MuxDownloader(
                boost::asio::io_service & io_svc);

            ~MuxDownloader();

            //打开一个下载链接
            boost::system::error_code open(
                std::string const & play_link, 
                std::string const & format, 
                std::string const & filename, 
                response_type const & resp);
            //关闭
             boost::system::error_code close(
                 boost::system::error_code & ec);

			 boost::system::error_code get_download_statictis(
                 DownloadStatistic & download_statistic,
                 boost::system::error_code & ec);

             bool is_free();

             //主线程
        private:
             void clear();

       //dispatch线程
        private:
            //call back
            void on_open(
                    boost::system::error_code const & ec);

            void on_setup(
                boost::system::error_code const & ec);

            void on_play(
                boost::system::error_code const & ec);

            void on_seek(
                boost::system::error_code const & ec);

        //aviable list
        private:
            boost::asio::io_service * io_svc_;
			ppbox::mux::Dispatcher * dispatcher_;

            boost::uint32_t session_id_;
            ppbox::mux::Sink* sink_;

            boost::uint32_t seek_time_; //计算出seek的值
            boost::uint32_t seek_size_; //计算出seek的值

            boost::uint32_t systime_;// = (Time::now() - stat_.begin_time).total_seconds();
            boost::uint32_t duration_;
            std::string filename_;
            bool working_;

            boost::system::error_code ec_;
        };

    } // namespace rtspd
} // namespace ppbox

#endif // _PPBOX_DOWNLOAD_MUX_DOWNLOADHER_H_
