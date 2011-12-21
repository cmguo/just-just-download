#include "ppbox/download/Common.h"
#include "ppbox/download/DownloadSink.h"
#include "ppbox/download/FileParse.h"
#include "ppbox/download/UpReport.h"

#include <ppbox/demux/DemuxerBase.h>
#include <boost/filesystem/operations.hpp>
#include <ppbox/demux/DemuxerError.h>

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
            std::string filename
            ,UpReport* upobj
            ,boost::uint32_t iTime 
            ,boost::uint32_t iSeek) //file seek的位置
            :upobj_(upobj)
            ,filename_(filename)
            ,time_(iTime)
        {
            try
            {
                std::string temp = filename.c_str();
                temp += ".tmp";
                if(iSeek)
                {
                    file_.open(temp.c_str(),std::ios::binary | std::ios::in | std::ios::out);
                    file_.seekp(iSeek,std::ios::beg);
                }
                else
                {
                    file_.open(temp.c_str(),std::ios::binary | std::ios::trunc | std::ios::out);
                }
                
            }
            catch(...)
            {
                std::cout<<"Open "<<filename<<" Error"<<std::endl;
            }
        }

        boost::system::error_code DownloadSink::on_finish( 
            boost::system::error_code const &ec)
        {
            try
            {
                if (!ec || ec == ppbox::demux::error::no_more_sample)
                {                
                    if(file_.is_open())
                        file_.close();

                    FileParse  pasr;
                    pasr.rename(filename_);
                    return boost::system::error_code();
                }
            }
            catch(...)
            {
                return error::save_file_error;
            }
            

        }

        //工作线程调用
        boost::system::error_code DownloadSink::write( ppbox::demux::Sample& tag)
        {
            boost::system::error_code ec;
            try
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
                        total_size += size;
                    }

                    //上报下载信息
                    upobj_->up_report(ReportData(total_size,tag.time));
                }
                return ec;
            }
            catch(...)
            {
                ec = error::save_file_error;
                return ec;
            }

        }

    } // namespace download
} // namespace ppbox
