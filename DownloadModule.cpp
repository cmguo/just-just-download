// DemuxerModule.cpp

#include "just/download/Common.h"
#include "just/download/DownloadModule.h"
#include "just/download/DispatchDownloader.h"
#include "just/download/Version.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>
using namespace boost::system;


#define dumpinfo(x) dump_info(x, __func__, __LINE__)


FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.download.DownloadModule", framework::logger::Debug);

namespace just
{
    namespace download
    {
        struct DownloadModule::DownloadInfo
        {
            bool detached;
            bool opened;
            bool busy;
            Downloader * downloader;
            framework::string::Url url;
            DownloadModule::open_response_type resp;
            error_code ec;

            DownloadInfo(
                Downloader * downloader)
                : downloader(downloader)
                , detached(false)
                , opened(false)
                , busy(false)
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
                    DownloadInfo const * info)
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
            std::vector<DownloadInfo *>::iterator iter = demuxers_.begin();
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
            DownloadInfo * info = create(url, resp, ec);
            boost::mutex::scoped_lock lock(mutex_);
            demuxers_.push_back(info);
            if (ec) {
                io_svc().post(boost::bind(resp, ec, info->downloader));
            } else {
                lock.unlock();
                async_open(info);
            }
            return info->downloader;
        }

        void DownloadModule::open(
            Downloader * downloader,
            framework::string::Url const & url, 
            open_response_type const & resp)
        {
            error_code ec;
            boost::mutex::scoped_lock lock(mutex_);
            DownloadInfo * info = NULL;
            std::vector<DownloadInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DownloadInfo::Finder(downloader));
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                info = (*iter);
                if((!info->opened) && (!info->detached) && (!info->busy)) {
                    info->url = url;
                    info->resp = resp;
                    dumpinfo(info);
                } else {
                    ec = boost::asio::error::operation_aborted;
                }
            }
            if (ec) {
                io_svc().post(boost::bind(resp, ec, downloader));
                return;
            } else {
                info->busy = true;
            }
            lock.unlock();
            async_open(info);
            return;
        }

        void DownloadModule::start(
            Downloader * downloader, long start, long end,
            open_response_type const & resp)
        {
            error_code ec;
            boost::mutex::scoped_lock lock(mutex_);
            DownloadInfo * info = NULL;
            std::vector<DownloadInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DownloadInfo::Finder(downloader));
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                info = (*iter);
                if((info->opened) && (!info->detached) && (!info->busy))
                {
                    info->busy = true;
                    info->resp = resp;
                } else {
                    ec = framework::system::logic_error::not_supported;
                }
                dumpinfo(info);
            }
            if (ec) {
                io_svc().post(boost::bind(resp, ec, downloader));
                return;
            }
            async_start(downloader, start, end);
            return;
        }

        bool DownloadModule::close(
            Downloader * downloader, 
            error_code & ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DownloadInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DownloadInfo::Finder(downloader));
            //assert(iter != demuxers_.end());
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                dumpinfo(*iter);
                if(!(*iter)->detached) {
                    (*iter)->detached = true;
                    close_locked(*iter, false, ec);
                } else {
                    ec = framework::system::logic_error::not_supported;
                }
            }
            return !ec;
        }

        bool DownloadModule::cancel(
            Downloader * downloader, 
            error_code & ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DownloadInfo *>::const_iterator iter = 
                std::find_if(demuxers_.begin(), demuxers_.end(), DownloadInfo::Finder(downloader));
            //assert(iter != demuxers_.end());
            if (iter == demuxers_.end()) {
                ec = framework::system::logic_error::item_not_exist;
            } else {
                dumpinfo(*iter);
                if ((*iter)->busy) {
                    downloader->cancel(ec);
                } else {
                    ec = framework::system::logic_error::not_supported;
                }
            }
            return !ec;
        }

        DownloadModule::DownloadInfo * DownloadModule::create(
            framework::string::Url const & url, 
            open_response_type const & resp, 
            error_code & ec)
        {
            Downloader * downloader = new DispatchDownloader(io_svc());
            DownloadInfo * info = new DownloadInfo(downloader);
            info->url = url;
            info->resp = resp;
            return info;
        }

        Downloader * DownloadModule::create()
        {
            Downloader * downloader = new DispatchDownloader(io_svc());
            DownloadInfo * info = new DownloadInfo(downloader);
            boost::mutex::scoped_lock lock(mutex_);
            demuxers_.push_back(info);
            return downloader;
        }


        void DownloadModule::async_open(
            DownloadInfo * info)
        {
            Downloader * downloader = info->downloader;
            downloader->open(
                info->url, 
                boost::bind(&DownloadModule::handle_open, this, _1, info));
        }

        void DownloadModule::async_start(
            Downloader * downloader, long start, long end)
        {
            downloader->start(start, end, 
                boost::bind(&DownloadModule::handle_start, this, _1, downloader));
        }
        
        void DownloadModule::handle_open(
            error_code const & ecc,
            DownloadInfo * info)
        {
            boost::mutex::scoped_lock lock(mutex_);

            error_code ec = ecc;
            
            Downloader * downloader = info->downloader;

            open_response_type resp;
            resp.swap(info->resp);
            //要放在close_locked之前对info->ec赋值
            info->ec = ec;

            info->busy = false;

            if(!ec)
                info->opened = true;
            
            dumpinfo(info);
            if(info->detached)
            {
                close_locked(info, true, ec);
            }
            lock.unlock();

            resp(ec, downloader);
        }

        void DownloadModule::handle_start(
            error_code const & ecc,
            Downloader * downloader)
        {
            error_code ec = ecc;
            open_response_type resp;
            {
                boost::mutex::scoped_lock lock(mutex_);
                std::vector<DownloadInfo *>::const_iterator iter = 
                    std::find_if(demuxers_.begin(), demuxers_.end(), DownloadInfo::Finder(downloader));
                //assert(iter != demuxers_.end());
                if (iter == demuxers_.end()) {
                    ec = framework::system::logic_error::item_not_exist;
                    return;
                } else {
                    (*iter)->busy = false;
                    resp.swap((*iter)->resp);
                    dumpinfo(*iter);
                    if((*iter)->detached)
                    {
                        close_locked((*iter), true, ec);
                    }
    
                }
            }
            resp(ec,downloader);
        }

        error_code DownloadModule::close_locked(
            DownloadInfo * info, 
            bool inner_call, 
            error_code & ec)
        {
            dumpinfo(info);
            if((info->detached) && (!info->busy)) {
                close(info, ec);
                destory(info);
            } else if (info->busy) {
                cancel(info, ec);
            }
            return ec;
        }

        error_code DownloadModule::cancel(
            DownloadInfo * info, 
            error_code & ec)
        {
            Downloader * downloader = info->downloader;
            if (downloader)
                downloader->cancel(ec);
            return ec;
        }

        error_code DownloadModule::close(
            DownloadInfo * info, 
            error_code & ec)
        {
            Downloader * downloader = info->downloader;
            if (downloader)
                downloader->close(ec);
            return ec;
        }

        void DownloadModule::destory(
            DownloadInfo * info)
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

        void DownloadModule::dump_info(DownloadInfo * info,
            char const * func, 
            int line)
        {
            LOG_INFO("fun " << func << " line " << line << " opened "<<info->opened<< " detached "<<info->detached<< " busy  "<<info->busy);
        }

        Downloader * DownloadModule::find(
            framework::string::Url const & url)
        {
            boost::mutex::scoped_lock lock(mutex_);
            std::vector<DownloadInfo *>::const_iterator iter = demuxers_.begin();
            for (size_t i = demuxers_.size() - 1; i != (size_t)-1; --i) {
                if ((*iter)->url == url) {
                    return (*iter)->downloader;
                }
            }
            return NULL;
        }

    } // namespace download
} // namespace just
