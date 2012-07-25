// RtspSession.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/MuxDownloader.h"
#include "ppbox/download/DownloadSink.h"
#include "ppbox/download/PutSink.h"
#include "ppbox/download/FileParse.h"

#include <ppbox/mux/Muxer.h>
#include <ppbox/demux/DemuxerError.h>
#include <ppbox/mux/tool/Dispatcher.h>

#include <framework/timer/TickCounter.h>
#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("MuxDownloader", 0)

namespace ppbox
{
    namespace download
    {

        MuxDownloader::MuxDownloader(
            boost::asio::io_service & io_svc)
            : io_svc_(&io_svc)
            , sink_(NULL)
            , seek_time_(0)
            , seek_size_(0)
            , systime_(0)
            , duration_(0)
        {
            clear();
        }

        MuxDownloader::~MuxDownloader()
        {
        }

        void MuxDownloader::clear()
        {
            sink_ = NULL;
            working_ = false;
            ec_.clear();
        }
        
        //打开一个下载链接
        boost::system::error_code MuxDownloader::open(
            std::string const & play_link, 
            std::string const & format, 
            std::string  const & filename, 
            response_type const & resp)
        {
            boost::system::error_code ec;

            working_ = true;

            //获取seek的time值
            FileParse file;
            seek_time_ = 0;
            seek_size_ = 0;

            ec = file.get_seektime(format,filename_,seek_time_,seek_size_);
            if(!ec)
            {
                LOG_S(Logger::kLevelAlarm, "MuxDownloader::open, ec:"<<ec.message());
            }

            return dispatcher_->open(session_id_,play_link,format,true, boost::bind(&MuxDownloader::on_open,this,_1));
        }
        //关闭
        boost::system::error_code MuxDownloader::close(
            boost::system::error_code & ec)
        {
            clear();
            ec = dispatcher_->close(session_id_);
            return ec;
        }

		boost::system::error_code MuxDownloader::get_download_statictis(
            DownloadStatistic & download_statistic,
            boost::system::error_code & ec)
        {
            // TODO:
            return ec;
        }

//call back
        void MuxDownloader::on_open(
            boost::system::error_code const & ec)
        {
            //set up 创建 sink
            if (!ec)
            {
				// TODO:
                //const ppbox::mux::MediaFileInfo & infoTemp = cur_mov_->muxer->mediainfo();
                //duration_ = infoTemp.duration;


                std::cout<<"Total Time:"<<duration_<<std::endl;

                assert(NULL == sink_);

                if (0 != strncmp(filename_.c_str(),"http",4))
                {
                    sink_ = new DownloadSink(filename_, seek_time_,seek_size_);
                }
                else
                {
                    sink_ = new PutSink(*io_svc_, filename_);
                }
                //分两种
                dispatcher_->setup(session_id_, sink_, 
                    boost::bind(&MuxDownloader::on_setup,this,_1));
            }
            else
            {
                ec_ = ec;
            }
        }

        void MuxDownloader::on_setup(
            boost::system::error_code const & ec)
        {
            if (!ec)
            {
                if (seek_time_)
                {//seek+play
                    dispatcher_->seek(session_id_, seek_time_, 0,
                        boost::bind(&MuxDownloader::on_seek,this,_1));
                }
                else
                {
                 //play
                    dispatcher_->record(session_id_, 
                        boost::bind(&MuxDownloader::on_play,this,_1));
                }
            }
            else
            {
                ec_ = ec;
            }
        }

        void MuxDownloader::on_seek(
            boost::system::error_code const & ec)
        {
            if (!ec)
            {
                dispatcher_->record(session_id_, 
                    boost::bind(&MuxDownloader::on_play,this,_1));
            }
            else
            {
                ec_ = ec;
            }
        }

        void MuxDownloader::on_play(
            boost::system::error_code const & ec)
        {
            //判断ec 的值  是为何结束?
            //退出下载时
            clear();
            LOG_S(Logger::kLevelEvent, "[MuxDownloader::on_play] ec:"<<ec.message());
            ec_ = ec;
            if (ec_ == ppbox::demux::error::no_more_sample)
            {
                ec_.clear();
            }
        }
        
    } // namespace download
} // namespace ppbox
