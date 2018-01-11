// DispatchDownloader.cpp

#include "just/download/Common.h"
#include "just/download/DispatchDownloader.h"

#include <just/dispatch/DispatchModule.h>
#include <just/dispatch/DispatcherBase.h>

#include <util/stream/UrlSink.h>
#include <util/stream/UrlSource.h>

#include <framework/timer/TimeCounter.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("just.download.DispatchDownloader", framework::logger::Debug)

namespace just
{
    namespace download
    {

        DispatchDownloader::DispatchDownloader(
            boost::asio::io_service & io_svc)
            : Downloader(io_svc)
            //, url_source_(NULL)
            , url_sink_(NULL)
            , opened_(false)
        {
            just::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<just::dispatch::DispatchModule>(io_svc);
            dispatcher_ = disp_mod.alloc_dispatcher(false);
        }

        DispatchDownloader::~DispatchDownloader()
        {
            //if (url_source_) {
            //    delete url_source_;
            //}
            if (url_sink_) {
                delete url_sink_;
            }
            just::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<just::dispatch::DispatchModule>(io_svc());
            disp_mod.free_dispatcher(dispatcher_);
        }

        void DispatchDownloader::open(
            framework::string::Url const & url,
            response_type const & resp)
        {
            url_ = url;
            url_.param("dispatch.fast", "true");
            Downloader::set_response(resp);
            just::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<just::dispatch::DispatchModule>(io_svc());
            boost::system::error_code ec;
            disp_mod.normalize_url(url_, ec);
            //url_source_ = util::stream::UrlSourceFactory::create(io_svc, url.protocol());
            //url_source_->async_open(url_, 
            //    boost::bind(&DispatchDownloader::handle_source, this ,_1));
            url_sink_ = util::stream::UrlSinkFactory::create(io_svc(), url_.protocol(), ec);
            if (url_sink_) {
                url_sink_->async_open(url_, 
                    just::avbase::invalid_size, 
                    just::avbase::invalid_size, 
                    boost::bind(&DispatchDownloader::handle_sink_open, this ,_1));
            } else {
                Downloader::response(ec);
            }
        }


        void DispatchDownloader::start(
            long start, long end,
            response_type const & resp)
        {
            boost::system::error_code ec;
            set_start_response(resp);
            just::dispatch::SeekRange range;
            range.type = just::dispatch::SeekRange::byte;
            range.beg = url_sink_->total(ec);
            if (range.beg == 0) {
                range.type = just::dispatch::SeekRange::none;
            }
            calc_speed(range.beg);
            dispatcher_->async_play(
                range, 
                just::dispatch::response_t(), 
                boost::bind(&DispatchDownloader::handle_play, this, _1));
        }


        bool DispatchDownloader::cancel(
            boost::system::error_code & ec)
        {
            io_svc().post(boost::bind(&just::dispatch::DispatcherBase::cancel,dispatcher_, ec));
            return true;
        }

        bool DispatchDownloader::close(
            boost::system::error_code & ec)
        {
            if (url_sink_) {
                url_sink_->close(ec);
            }
            return dispatcher_->close(ec);
        }

        bool DispatchDownloader::get_statictis(
            DownloadStatistic & stat,
            boost::system::error_code & ec)
        {
            if (!opened_) {
                stat.total_size = 0;
                stat.finish_size = 0;
                stat.speed = 0;
                ec.clear();
                return true;
            }
            just::avbase::StreamStatus info;
            if (!dispatcher_->get_stream_status(info, ec))
                return false;
            stat.total_size = info.byte_range.end;
            stat.finish_size = info.byte_range.pos;
            stat.speed = calc_speed(info.byte_range.pos);
            return true;
        }

        //void DispatchDownloader::handle_source_open(
        //    boost::system::error_code const & ec)
        //{
        //    LOG_INFO("[handle_source_open] ec:" << ec.message());
        //}

        void DispatchDownloader::handle_sink_open(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_sink_open] ec:" << ec.message());

            if (ec) {
                response(ec);
                return;
            }

            dispatcher_->async_open(
                url_, 
                boost::bind(&DispatchDownloader::handle_dispatcher_open, this ,_1));
        }

        void DispatchDownloader::handle_dispatcher_open(
            boost::system::error_code ec)
        {
            LOG_INFO("[handle_dispatcher_open] ec:" << ec.message());

            if (!ec) {
                url_sink_->set_non_block(true, ec);
                if (ec == framework::system::logic_error::not_supported) {
                    ec.clear();
                }
            }

            if (!ec) {
                dispatcher_->setup(-1, *url_sink_, ec);
            }
#if 1
            opened_ = true;
            response(ec);
            return;
#else

            if (ec) {
                response(ec);
                return;
            }

            opened_ = true;
            just::dispatch::SeekRange range;
            range.type = just::dispatch::SeekRange::byte;
            range.beg = url_sink_->total(ec);
            if (range.beg == 0) {
                range.type = just::dispatch::SeekRange::none;
            }
            calc_speed(range.beg);
            dispatcher_->async_play(
                range, 
                just::dispatch::response_t(), 
                boost::bind(&DispatchDownloader::handle_play, this, _1));
#endif            
        }

        void DispatchDownloader::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_play] ec:" << ec.message());
            start_response(ec);
        }

    } // namespace download
} // namespace just
