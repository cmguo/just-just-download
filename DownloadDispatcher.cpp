// RtspSession.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/DownloadDispatcher.h"
#include "ppbox/download/DownloadSink.h"
#include "ppbox/download/FileParse.h"


#include <ppbox/mux/Muxer.h>
#include <ppbox/demux/DemuxerError.h>

#include <framework/timer/TickCounter.h>
#include <boost/bind.hpp>
#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("DownloadDispatcher", 0)

namespace ppbox
{
    namespace download
    {
        DownloadDispatcher::DownloadDispatcher(
            util::daemon::Daemon & daemon)
            : ppbox::mux::Dispatcher(daemon)
            ,sink_(NULL)
            ,seek_time_(0)
            ,seek_size_(0)
            ,systime_(0)
            ,duration_(0)
        {
            status_ = new FileDownloadStatistic();
            clear();
        }

        DownloadDispatcher::~DownloadDispatcher()
        {
            delete status_;
        }

        void DownloadDispatcher::clear()
        {
            sink_ = NULL;
            working_ = false;
            status_->clear();
            ec_.clear();
            
        }
        
        //打开一个下载链接
        boost::system::error_code DownloadDispatcher::open(
            std::string const & play_link
            ,std::string const & format
            ,std::string const & dst
            ,std::string  const & filename)
        {
            boost::system::error_code ec;
            filename_ = dst +filename;
            working_ = true;

            //获取seek的time值
            FileParse file;
            seek_time_ = 0;
            seek_size_ = 0;

            ec = file.get_seektime(format,filename_,seek_time_,seek_size_);
            if(!ec)
            {
                LOG_S(Logger::kLevelAlarm, "DownloadDispatcher::open, ec:"<<ec.message());
            }

            return Dispatcher::open(session_id_,play_link,format,true, boost::bind(&DownloadDispatcher::on_open,this,_1));
        }
        //关闭
        boost::system::error_code DownloadDispatcher::close()
        {
            clear();
            return Dispatcher::close(session_id_);
        }

        void DownloadDispatcher::get_download_statictis(
            FileDownloadStatistic & download_statistic,
            boost::system::error_code & ec)
        {
            download_statistic = *status_;
        }

//call back
        void DownloadDispatcher::on_open(
            boost::system::error_code const & ec)
        {
            //set up 创建 sink
            if (!ec)
            {
                const ppbox::mux::MediaFileInfo & infoTemp = cur_mov_->muxer->mediainfo();
                duration_ = infoTemp.duration;


                std::cout<<"Total Time:"<<duration_<<std::endl;

                assert(NULL == sink_);
                sink_ = new DownloadSink(filename_,this,seek_time_,seek_size_);
                Dispatcher::setup(session_id_
                    ,sink_ 
                    ,boost::bind(&DownloadDispatcher::on_setup,this,_1));
            }
            else
            {
                ec_ = ec;
            }
        }

        void DownloadDispatcher::on_setup(
            boost::system::error_code const & ec)
        {
            if (!ec)
            {
                if (seek_time_)
                {//seek+play
                    Dispatcher::seek(session_id_,seek_time_,0
                        ,boost::bind(&DownloadDispatcher::on_seek,this,_1));
                }
                else
                {
                 //play
                    Dispatcher::record(session_id_
                        ,boost::bind(&DownloadDispatcher::on_play,this,_1));
                }
            }
            else
            {
                ec_ = ec;
            }
        }

        void DownloadDispatcher::on_seek(
            boost::system::error_code const & ec)
        {
            if (!ec)
            {
                Dispatcher::record(session_id_
                    ,boost::bind(&DownloadDispatcher::on_play,this,_1));
            }
            else
            {
                ec_ = ec;
            }
        }

        void DownloadDispatcher::on_play(
            boost::system::error_code const & ec)
        {
            //判断ec 的值  是为何结束?
            //退出下载时
            //clear();
            LOG_S(Logger::kLevelEvent, "[DownloadDispatcher::on_play] ec:"<<ec.message());
            ec_ = ec;
            status_->speed = 0;
            if (ec_ == ppbox::demux::error::no_more_sample)
            {
                status_->finish_percent = 1.0;
                ec_.clear();
            }

        }

        bool DownloadDispatcher::is_free()
        {
            return (!working_);
        }

        void DownloadDispatcher::on_up_report(ReportData const & rdata)
        {
            //std::cout<<"on_up_report:"<<boost::this_thread::get_id()<<std::endl;
            //根据上报的数据，计算 spend finish_size 百分之

            boost::uint32_t curr_time = framework::timer::TickCounter::tick_count();
            if(0 != systime_)
            {
                if(curr_time - systime_ > 0)
                    status_->speed = rdata.curr_data_size*1000/(curr_time - systime_);
            }
            status_->finish_percent = (float)rdata.curr_time/duration_;
            systime_ = curr_time;
            status_->finish_size += rdata.curr_data_size;
        }

        //线程中
        void DownloadDispatcher::up_report(ReportData const & rdata)
        {
            //std::cout<<"up_report:"<<boost::this_thread::get_id()<<std::endl;
            get_daemon().io_svc().post(boost::bind(&DownloadDispatcher::on_up_report,this,rdata));
        }

        
    } // namespace download
} // namespace ppbox
