// Mp4Manager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Mp4Dispatcher.h"
#include "ppbox/download/FileParse.h"

#ifndef PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/Certifier.h>
#endif

#include <util/protocol/pptv/Url.h>
#include <boost/filesystem/operations.hpp>

#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::logger;

#include <fstream>

FRAMEWORK_LOGGER_DECLARE_MODULE("Mp4Dispatcher");

namespace ppbox
{
    namespace download
    {
        Mp4Dispatcher::Mp4Dispatcher(util::daemon::Daemon & daemon)
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            :bigmp4_(daemon.io_svc())
            ,head_size_(0)
#else
            :head_size_(0)
#endif
            ,status_(NULL)
            ,timer_(daemon.io_svc() )
        {
            status_ = new FileDownloadStatistic();
        }

        Mp4Dispatcher::~Mp4Dispatcher()
        {
            timer_.cancel();
            delete status_;
            status_ = NULL;
        }


        boost::system::error_code Mp4Dispatcher::add(
            std::string const & play_link
            ,std::string const & format
            ,std::string const & dst
            ,std::string  const & filename)
        {
            LOG_S(Logger::kLevelEvent, "[add] play_link:"<<play_link
                <<" format:"<<format
                <<" filename:"<<filename);

            boost::system::error_code ec;
            filename_ = dst +filename;

            std::string tmpfile= filename_+".tmp";

            FileParse pasr;
            if(!pasr.is_exist(tmpfile))
            {
                std::ofstream stream(tmpfile.c_str());
                stream.close();
            }

            file_.open(tmpfile.c_str(),std::ios::in|std::ios::out|std::ios::binary);

            std::string oldUrl(play_link);
            oldUrl = oldUrl.substr(std::string("ppvod://").size());

            oldUrl = parse_url(oldUrl,ec);
#if !defined(PPBOX_DISABLE_VOD)
            bigmp4_.async_open(oldUrl
                ,ppbox::vod::BigMp4::FetchMode::big_head
                //,ppbox::vod::BigMp4::FetchMode::small_head
                ,file_
                ,boost::bind(&Mp4Dispatcher::async_open_callback,this,_1));
#elif !defined(PPBOX_DISABLE_PEER)
            bigmp4_.async_open(oldUrl
                ,ppbox::peer::BigMp4::FetchMode::big_head
                ,file_
                ,boost::bind(&Mp4Dispatcher::async_open_callback,this,_1));
#else
            ec = error::not_support_download;
#endif
            return ec;
        }

        void Mp4Dispatcher::async_open_callback(boost::system::error_code const & ec)
        {
            //
            if(ec)
            {
                LOG_S(Logger::kLevelEvent, "[async_open_callback] ec:"<<ec.message());
            }
            else
            {
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
                // 对size 判断是下体还是下header
                boost::uint32_t downSize = 0 ;
                bigmp4_.get_valid_size(downSize);
                file_.seekp(downSize,std::ios::beg);

                bigmp4_.async_tranfer(
                    downSize
                    ,file_
                    ,boost::bind(&Mp4Dispatcher::async_body_callback,this,_1));

                handle_timer(ec); //每隔一秒统计一次
#endif
            }
        }

        void Mp4Dispatcher::handle_timer( boost::system::error_code const & ec)
        {
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            boost::uint32_t total_size = 0;
            boost::uint32_t vaild_size = 0;
            bigmp4_.get_valid_size(vaild_size);
            bigmp4_.get_total_size(total_size);

            if (total_size == 0 || NULL == status_)
            {
                return;
            }

            if(vaild_size > total_size) 
            {
                assert(0);
                total_size = vaild_size;
            }

            status_->finish_percent = (float)vaild_size/total_size;
            status_->speed = vaild_size - status_->finish_size;
            status_->finish_size = vaild_size;

            timer_.expires_from_now(boost::posix_time::seconds(1));
            timer_.async_wait(boost::bind(&Mp4Dispatcher::handle_timer, this, _1));
#endif
        }

        void Mp4Dispatcher::async_body_callback(boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelEvent, "[async_body_callback] ec:"<<ec.message());

            file_.close();
            if(!ec)
            {
                FileParse  pasr;
                pasr.rename(filename_);
            }
        }

        boost::system::error_code Mp4Dispatcher::del(boost::system::error_code & ec)
        {
            LOG_S(Logger::kLevelEvent, "[del]");
#if !defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER)
            bigmp4_.close();
#endif
            return ec;
        }

        boost::system::error_code Mp4Dispatcher::get_download_statictis(
            FileDownloadStatistic & download_statistic,
            boost::system::error_code & ec)
        {
            download_statistic = *status_;
            return ec;
        }

        std::string Mp4Dispatcher::parse_url(std::string const &url,boost::system::error_code& ec)
        {
            std::string  newUrl;
            std::string key;
#ifdef PPBOX_DISABLE_CERTIFY
            key = "kioe257ds";
#else
            //ppbox::certify::Certifier& cert = util::daemon::use_module<ppbox::certify::Certifier>(global_daemon());
            //cert.certify_url(ppbox::certify::CertifyType::vod,"",key,ec);
            if (ec)
            {
                LOG_S(framework::logger::Logger::kLevelError,"[parse_url] ec:"<<ec.message());
                return newUrl;
            }
#endif
            newUrl = util::protocol::pptv::url_decode(url, key);

            framework::string::StringToken st(newUrl, "||");
            if (!st.next_token(ec)) {
                newUrl = st.remain();
            }

            return newUrl;
        }
    }
}
