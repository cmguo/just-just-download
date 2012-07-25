// DownloadSink.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/DownloadSink.h"

#include <ppbox/demux/base/DemuxerBase.h>
#include <ppbox/demux/base/DemuxerError.h>


FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("DownloadSink", 0)

namespace ppbox
{
    namespace download
    {
        DownloadSink::~DownloadSink()
        {
            if(file_.is_open())
                file_.close();
        }

        DownloadSink::DownloadSink(
            std::string filename, 
            boost::uint32_t iTime, 
			boost::uint32_t iSeek)
			: time_(iTime)
        {
            if (iSeek)
            {
                file_.open(filename.c_str(),std::ios::binary | std::ios::in | std::ios::out);
                file_.seekp(iSeek,std::ios::beg);
            }
            else
            {
                file_.open(filename.c_str(),std::ios::binary | std::ios::trunc | std::ios::out);
            }
        }

        boost::system::error_code DownloadSink::on_finish( 
            boost::system::error_code const &ec)
        {
            if (file_.is_open())
                file_.close();
            return ec;
        }

        //工作线程调用
        size_t DownloadSink::write(
            boost::posix_time::ptime const & time_send,
            ppbox::demux::Sample& tag,
            boost::system::error_code& ec)
        {
            boost::uint32_t total_size = 0;
            if ( tag.time >= time_)
            {
                //可以写文件的条件 在else实现了
                for(boost::uint32_t i = 0; i < tag.data.size(); ++i) 
                {
                    boost::asio::const_buffer & buf = tag.data.at(i);
                    boost::uint8_t const * data = boost::asio::buffer_cast<boost::uint8_t const *>(buf);
                    boost::uint32_t size = boost::asio::buffer_size(buf);
                    file_.write((const char*)data,size);
                    total_size  += size;
                    time_ = tag.time;
                }
            }
            return 0;
        }

    } // namespace download
} // namespace ppbox
