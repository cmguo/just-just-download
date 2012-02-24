#include "ppbox/download/Common.h"
#include "ppbox/download/PutSink.h"
#include "ppbox/download/FileParse.h"
#include "ppbox/download/UpReport.h"

#include <ppbox/demux/base/DemuxerBase.h>
#include <ppbox/demux/base/DemuxerError.h>

#include <framework/string/Base16.h>

#include <util/buffers/BufferSize.h>

#include <boost/filesystem/operations.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("DownloadSink", 0)

namespace ppbox
{
    namespace download
    {
        PutSink::~PutSink()
        {
            http_.close();
        }

        PutSink::PutSink(
            boost::asio::io_service & io_svc
            ,UpReport* upobj
            ,std::string http_file) //file seek的位置
            :http_(io_svc)
            ,socket_(http_)
            ,upobj_(upobj)
            ,http_file_(http_file)
            ,bRunning(false)
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
        boost::system::error_code PutSink::write( 
            boost::posix_time::ptime const & time_send,
            ppbox::demux::Sample& tag)
        {
            boost::system::error_code ec;

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
                    return ec;
                }
                bRunning = true;
            }

            boost::uint32_t total_size = util::buffers::buffer_size(tag.data);
            boost::asio::write(
                //socket_, 
                http_, 
                tag.data, 
                boost::asio::transfer_all(), 
                ec);

            //上报下载信息
            upobj_->up_report(ReportData(total_size,tag.time));
            return ec;

        }
    } // namespace download
} // namespace ppbox
