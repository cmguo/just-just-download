// PutSink.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/PutSink.h"

#include <ppbox/demux/base/DemuxerBase.h>
#include <ppbox/demux/base/DemuxerError.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.download.PutSink", Debug)

namespace ppbox
{
    namespace download
    {
        PutSink::~PutSink()
        {
            http_.close();
        }

        PutSink::PutSink(
            boost::asio::io_service & io_svc, 
            std::string http_file) //file seek的位置
            : DownloadSink(http_file, 0, 0)
            , http_(io_svc)
            , socket_(http_)
            , http_file_(http_file)
            , bRunning(false)
        {

        }

        boost::system::error_code PutSink::on_finish( 
            boost::system::error_code const &ec)
        {
            boost::system::error_code ec1;
            socket_.send(socket_.eof(),0,ec1);
            return ec1;
        }

        //工作线程调用
        size_t PutSink::write( 
            boost::posix_time::ptime const & time_send,
            ppbox::demux::Sample& tag,
            boost::system::error_code& ec)
        {
            if (!bRunning)
            {
                //framework::
                framework::string::Url url(http_file_);
                util::protocol::HttpRequestHead head(
                    util::protocol::HttpRequestHead::post
                    ,url.path_all()
                    ,0x00000101);
                //head["Transfer-Encoding"] = "{chunked}";
                head.host = url.host_svc();
                http_.open(head,ec);
                if (ec && ec.value() != 100)
                {
                    std::cout<<"PutSink::write ec:"<<ec.message()<<std::endl;
                    return 0;
                }
                bRunning = true;
            }

            return boost::asio::write(
                //socket_, 
                http_, 
                tag.data, 
                boost::asio::transfer_all(), 
                ec);
        }
    } // namespace download
} // namespace ppbox
