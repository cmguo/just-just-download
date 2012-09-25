// Manager.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Manager.h"
#include "ppbox/download/Mp4Downloader.h"

#include <framework/system/LogicError.h>

#include <utility>

#include <boost/bind.hpp>

using namespace boost::system;

namespace ppbox
{
    namespace download
    {
        Manager::Manager(util::daemon::Daemon & daemon)
#ifdef  PPBOX_DISABLE_CERTIFY
            : ppbox::common::CommonModuleBase<Manager>(daemon, "download")
#else
            : ppbox::certify::CertifyUserModuleBase<Manager>(daemon, "download")
#endif
        {
        }

        Manager::~Manager()
        {
        }

        error_code Manager::startup()
        {
        #ifndef  PPBOX_DISABLE_CERTIFY
            start_certify();
        #endif
            return error_code();
        }

        void Manager::shutdown()
        {
            boost::mutex::scoped_lock lc(mutex_);

         #ifndef  PPBOX_DISABLE_CERTIFY
            stop_certify();
         #endif
            std::vector<ppbox::download::DownloadInfo>::iterator iter = info_vec_.begin();
            error_code ec;
            while(info_vec_.end() != iter) {
                if (iter->cur_status == working) {
                    iter->cur_status = canceling;
                    iter->downloader->close(ec);
                    ++iter;
                } else if (stopped == iter->cur_status) {
                    iter->cur_status = deleted;
                    delete iter->downloader;
                    iter = info_vec_.erase(iter);
                    ec.clear();
                } else {
                    ++iter;
                }
            }
            while(!info_vec_.empty()) {
                cond_.wait(lc);
            }         
        }

        // 进入认证成功状态
        void Manager::certify_startup()
        {
            std::cout << "certify_startup" << std::endl;
        }

        // 退出认证成功状态
        void Manager::certify_shutdown(
            boost::system::error_code const & ec)
        {
            std::cout << "certify_shutdown" << std::endl;
        }

        // 认证出错
        void Manager::certify_failed(
            boost::system::error_code const & ec)
        {
            Manager::certify_shutdown(ec);
        }

        Downloader * Manager::add(
            std::string const & playlink,
            std::string const & format,
            std::string const & filename,
            error_code & ec,
            Downloader::response_type const & resp)
        {
            boost::mutex::scoped_lock lc(mutex_);

            DownloadInfo info;
            info.resp = resp;
            if(format == "mp4" ) {
                Mp4Downloader * mp4_download = new Mp4Downloader(io_svc());
                info.downloader = mp4_download;
                mp4_download->open(playlink,
                                format,
                                filename,
                                boost::bind(&Manager::add_call_back, this, info.downloader, _1));
                info.cur_status = working;
            } else {
                //flvts_module_.add(playlink,format,dest,filename,info->downloader,ec);
            }
            info_vec_.push_back(info);
            return info.downloader;
        }

        struct FindDownloader
        {
            FindDownloader(
                Downloader * downloader)
                : downloader_(downloader)
            {
            }
            
            bool operator()(DownloadInfo const & info) const
            {
                return downloader_ == info.downloader;
            }

        private:
            Downloader * downloader_;
        };

        void Manager::add_call_back(
            Downloader * downloader,
            boost::system::error_code const  & ec)
        {
            boost::mutex::scoped_lock lc(mutex_);

            std::vector<DownloadInfo>::iterator itr = 
                std::find_if(info_vec_.begin(), info_vec_.end(), FindDownloader(downloader));
            assert(itr != info_vec_.end());
            if (itr == info_vec_.end())
                return;

            itr->resp(ec);
            itr->error_code = ec;
            if (working == itr->cur_status) {
                itr->cur_status = stopped;
            } else if (canceling == itr->cur_status) {
                itr->cur_status = deleted;
                 delete downloader;
                info_vec_.erase(itr);
            }
        }

        error_code Manager::del(
            Downloader * downloader,
            error_code & ec)
        {
            boost::mutex::scoped_lock lc(mutex_);

            std::vector<DownloadInfo>::iterator itr = 
                std::find_if(info_vec_.begin(), info_vec_.end(), FindDownloader(downloader));
            assert(itr != info_vec_.end());
            if (itr == info_vec_.end())
                return ec = framework::system::logic_error::item_not_exist;

            DownloadInfo & info = *itr;
            if (working == info.cur_status) {
                info.cur_status = canceling;
                info.downloader->close(ec);
            } else if (stopped == info.cur_status) {
                info.cur_status = deleted;
                delete itr->downloader;
                info_vec_.erase(itr);
                ec.clear();
            } else if (canceling == info.cur_status) {
                ec = framework::system::logic_error::item_not_exist;
            }
            return ec;
        }

        error_code Manager::get_download_statictis(
            Downloader * downloader,
            DownloadStatistic & stat,
            error_code & ec)
        {
            boost::mutex::scoped_lock lc(mutex_);

            std::vector<DownloadInfo>::iterator itr = 
                std::find_if(info_vec_.begin(), info_vec_.end(), FindDownloader(downloader));
            assert(itr != info_vec_.end());
            if (itr == info_vec_.end())
                return ec = framework::system::logic_error::item_not_exist;

            if (!itr->error_code) {
                downloader->get_download_statictis(stat, ec);
            } else {
                ec = itr->error_code;
            }

            return ec;
        }
// 
//         boost::uint32_t Manager::get_downloader_count(void) const
//         {
// 
//             return (mp4_module_.get_downloader_count()+flvts_module_.get_downloader_count());
//         }

        error_code Manager::get_last_error(
                Downloader * downloader)
        {
            error_code ec;
            std::vector<DownloadInfo>::iterator itr = 
                std::find_if(info_vec_.begin(), info_vec_.end(), FindDownloader(downloader));
            if (itr == info_vec_.end()) {
                return ec;
            }
            return itr->error_code;

        }

        void Manager::set_download_path(
                char const * path)
        {

        }
    }
}
