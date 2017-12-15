// DemuxerModule.cpp

#include "just/download/Common.h"
#include "just/download/DownloadModule.h"
#include "just/download/DispatchDownloader.h"
#include "just/download/Version.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
using namespace boost::system;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.download.DownloadModule", framework::logger::Debug);

namespace just
{
    namespace download
    {

        struct DownloadModule::DemuxInfo
        {
            enum StatusEnum
            {
                closed, 
                opening, 
                canceled, 
                opened, 
            };

            StatusEnum status;
            Downloader * downloader;
            framework::string::Url url;
            DownloadModule::open_response_type resp;
            error_code ec;

            DemuxInfo(
                Downloader * downloader)
                : status(closed)
                , downloader(downloader)
            {
            }

            struct Finder
            {
                Finder(
                    Downloader * downloader)
                    : downloader_(downloader)
                {
                }

                bool operator()(
                    DemuxInfo const * info)
                {
                    return info->downloader == downloader_;
                }

            private:
                Downloader * downloader_;
            };
        };

        DownloadModule::DownloadModule(
            util::daemon::Daemon & daemon)
            : just::common::CommonModuleBase<DownloadModule>(daemon, "DemuxerModule")
        {
        }

        DownloadModule::~DownloadModule()
        {
        }

        bool DownloadModule::startup(
            error_code & ec)
        {
            return true;
        }

        bool DownloadModule::shutdown(
            error_code & ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DemuxInfo *>::iterator iter = demuxers_.begin();
            for (size_t i = demuxers_.size() - 1; i != (size_t)-1; --i) {
                close_locked(demuxers_[i], false, ec);
            }
            /*while (!demuxers_.empty()) {
                cond_.wait(lock);
            }*/
            return true;
        }

        Downloader * DownloadModule::open(
            framework::string::Url const & url, 
            open_response_type const & resp)
        {
            error_code ec;
            DemuxInfo * info = create(url, resp, ec);
            boost::mutex::scoped_lock lock(mutex_);
            demuxers_.push_back(info);
            if (ec) {
                io_svc().post(boost::bind(resp, ec, info->downloader));
            } else {
                async_open(lock, info);
            }
            return info->downloader;
        }

        bool DownloadModule::start(
            Downloader * downloader, long start, long end,
            open_response_type const & resp)
        {
            error_code ec;
            boost::mutex::scoped_lock lock(mutex_);
            
            std::vector<DemuxInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DemuxInfo::Finder(downloader));
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                if((*iter)->status != DemuxInfo::opened){
                    ec = framework::system::logic_error::not_supported;
                }
            }
            
            if (ec) {
                io_svc().post(boost::bind(resp, ec, downloader));
                return false;
            }
            async_start(downloader, start, end, resp);
            return true;
        }

        bool DownloadModule::close(
            Downloader * downloader, 
            error_code & ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DemuxInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DemuxInfo::Finder(downloader));
            //assert(iter != demuxers_.end());
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                close_locked(*iter, false, ec);
            }
            return !ec;
        }

        bool DownloadModule::cancel(
            Downloader * downloader, 
            error_code & ec)
        {
            LOG_INFO("fun " << __func__ << " line " << __LINE__ << " pid "<< getpid()<< " tid "<< gettid());
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DemuxInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DemuxInfo::Finder(downloader));
            //assert(iter != demuxers_.end());
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                if (downloader)
                    downloader->cancel(ec);
            }
            return !ec;
        }

        DownloadModule::DemuxInfo * DownloadModule::create(
            framework::string::Url const & url, 
            open_response_type const & resp, 
            error_code & ec)
        {
            Downloader * downloader = new DispatchDownloader(io_svc());
            DemuxInfo * info = new DemuxInfo(downloader);
            info->url = url;
            info->resp = resp;
            return info;
        }

        void DownloadModule::async_open(
            boost::mutex::scoped_lock & lock, 
            DemuxInfo * info)
        {
            Downloader * downloader = info->downloader;
            lock.unlock();
            downloader->open(
                info->url, 
                boost::bind(&DownloadModule::handle_open, this, _1, info));
            lock.lock();
            info->status = DemuxInfo::opening;
        }

        void DownloadModule::async_start(
            Downloader * downloader, long start, long end,
            open_response_type const & resp)
        {
            downloader->start(start, end, 
                boost::bind(&DownloadModule::handle_start, this, _1, downloader, resp));
        }
        
        void DownloadModule::handle_open(
            error_code const & ecc,
            DemuxInfo * info)
        {
            boost::mutex::scoped_lock lock(mutex_);

            error_code ec = ecc;
            
            Downloader * downloader = info->downloader;

            open_response_type resp;
            resp.swap(info->resp);
            //要放在close_locked之前对info->ec赋值
            info->ec = ec;
            if (info->status == DemuxInfo::canceled) {
                close_locked(info, true, ec);
                ec = boost::asio::error::operation_aborted;
            } else {
                info->status = DemuxInfo::opened;
            }

            lock.unlock();

            resp(ec, downloader);
        }

        void DownloadModule::handle_start(
            error_code const & ecc,
            Downloader * downloader,
            open_response_type const & resp)
        {
            error_code ec = ecc;
            resp(ec,downloader);
        }

        error_code DownloadModule::close_locked(
            DemuxInfo * info, 
            bool inner_call, 
            error_code & ec)
        {
            assert(!inner_call || info->status == DemuxInfo::opening || info->status == DemuxInfo::canceled);
            if (info->status == DemuxInfo::closed) {
                ec = error::not_open;
            } else if (info->status == DemuxInfo::opening) {
                info->status = DemuxInfo::canceled;
                cancel(info, ec);
            } else if (info->status == DemuxInfo::canceled) {
                if (inner_call) {
                    info->status = DemuxInfo::closed;
                    ec.clear();
                } else {
                    ec = error::not_open;
                }
            } else if (info->status == DemuxInfo::opened) {
                info->status = DemuxInfo::closed;
            }
            if (info->status == DemuxInfo::closed) {
                close(info, ec);
                destory(info);
            }
            return ec;
        }

        error_code DownloadModule::cancel(
            DemuxInfo * info, 
            error_code & ec)
        {
            Downloader * downloader = info->downloader;
            if (downloader)
                downloader->cancel(ec);
            return ec;
        }

        error_code DownloadModule::close(
            DemuxInfo * info, 
            error_code & ec)
        {
            Downloader * downloader = info->downloader;
            if (downloader)
                downloader->close(ec);
            return ec;
        }

        void DownloadModule::destory(
            DemuxInfo * info)
        {
            Downloader * downloader = info->downloader;
            if (downloader)
                delete downloader;
            demuxers_.erase(
                std::remove(demuxers_.begin(), demuxers_.end(), info), 
                demuxers_.end());
            delete info;
            info = NULL;
        }

        Downloader * DownloadModule::find(
            framework::string::Url const & url)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DemuxInfo *>::const_iterator iter = demuxers_.begin();
            for (size_t i = demuxers_.size() - 1; i != (size_t)-1; --i) {
                if ((*iter)->url == url) {
                    return (*iter)->downloader;
                }
            }
            return NULL;
        }

    } // namespace download
} // namespace just
