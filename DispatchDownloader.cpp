// DispatchDownloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/DispatchDownloader.h"

#include <ppbox/dispatch/DispatchModule.h>
#include <ppbox/dispatch/DispatcherBase.h>

#include <ppbox/data/base/UrlSource.h>
#include <ppbox/data/base/UrlSink.h>
#include <ppbox/data/sink/FileSink.h>

#include <framework/timer/TimeCounter.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.download.DispatchDownloader", framework::logger::Debug)

namespace ppbox
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
            ppbox::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<ppbox::dispatch::DispatchModule>(io_svc);
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
            ppbox::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<ppbox::dispatch::DispatchModule>(io_svc());
            disp_mod.free_dispatcher(dispatcher_);
        }

        void DispatchDownloader::open(
            framework::string::Url const & url,
            response_type const & resp)
        {
            url_ = url;
            Downloader::set_response(resp);
            ppbox::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<ppbox::dispatch::DispatchModule>(io_svc());
            boost::system::error_code ec;
            disp_mod.normalize_url(url_, ec);
            //url_source_ = ppbox::data::UrlSource::create(io_svc, url.protocol());
            //url_source_->async_open(url_, 
            //    boost::bind(&DispatchDownloader::handle_source, this ,_1));
            url_sink_ = ppbox::data::UrlSink::create(io_svc(), url_.protocol(), ec);
            if (url_sink_) {
                url_sink_->async_open(url_, 
                    boost::bind(&DispatchDownloader::handle_sink_open, this ,_1));
            } else {
                Downloader::response(ec);
            }
        }

        bool DispatchDownloader::cancel(
            boost::system::error_code & ec)
        {
            return dispatcher_->cancel(ec);
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
            ppbox::data::StreamStatus info;
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

            if (ec) {
                response(ec);
                return;
            }

            opened_ = true;

            ppbox::dispatch::SeekRange range;
            if (url_.protocol() == "file") {
                range.type = ppbox::dispatch::SeekRange::byte;
                ppbox::data::FileSink * file_sink = static_cast<ppbox::data::FileSink *>(url_sink_);
                file_sink->file_stream().seekp(0, std::ios::end);
                range.beg = file_sink->file_stream().tellp();
                if (range.beg == 0) {
                    range.type = ppbox::dispatch::SeekRange::none;
                }
            }
            calc_speed(range.beg);
            dispatcher_->async_play(
                range, 
                ppbox::dispatch::response_t(), 
                boost::bind(&DispatchDownloader::handle_play, this, _1));
        }

        void DispatchDownloader::handle_play(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_play] ec:" << ec.message());
            response(ec);
        }

    } // namespace download
} // namespace ppbox
