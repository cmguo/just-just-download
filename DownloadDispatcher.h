#ifndef _PPBOX_RTSPD_DOWNLOAD_DISPATCHER_H_
#define _PPBOX_RTSPD_DOWNLOAD_DISPATCHER_H_

#include "ppbox/download/UpReport.h"

#include <ppbox/mux/tool/Dispatcher.h>

#include <boost/asio/ip/tcp.hpp>

namespace ppbox
{
    namespace mux
    {
        struct MuxTag;
        class Sink;
    }

    namespace download
    {  
        class DownloadSink;
        struct FileDownloadStatistic;


        class DownloadDispatcher 
            : public ppbox::mux::Dispatcher, public UpReport
        {
        public:
            DownloadDispatcher( util::daemon::Daemon & daemon);
            ~DownloadDispatcher();

            //打开一个下载链接
            boost::system::error_code open(
                std::string const & play_link
                ,std::string const & format
                ,std::string const & dst
                ,std::string const & filename);
            //关闭
             boost::system::error_code close();

             void get_download_statictis(
                 FileDownloadStatistic & download_statistic,
                 boost::system::error_code & ec);

             bool is_free();

             //主线程
        private:
             void on_up_report(ReportData const & rdata);
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

            virtual void up_report(ReportData const & rdata);

        //aviable list
        private:
            boost::uint32_t session_id_;
            FileDownloadStatistic* status_;
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

#endif // _PPBOX_RTSPD_RTP_DISPATCHER_H_