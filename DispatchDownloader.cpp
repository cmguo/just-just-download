// DispatchDownloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/DispatchDownloader.h"

#include <ppbox/dispatch/DispatchModule.h>
#include <ppbox/dispatch/DispatcherBase.h>

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
            , sink_(NULL)
            , opened_(false)
        {
            sink_ = new ppbox::data::FileSink(io_svc);
            ppbox::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<ppbox::dispatch::DispatchModule>(io_svc);
            dispatcher_ = disp_mod.alloc_dispatcher(false);
        }

        DispatchDownloader::~DispatchDownloader()
        {
            ppbox::dispatch::DispatchModule & disp_mod = 
                util::daemon::use_module<ppbox::dispatch::DispatchModule>(io_svc());
            disp_mod.free_dispatcher(dispatcher_);
            delete sink_;
        }

        void DispatchDownloader::open(
            framework::string::Url const & url,
            response_type const & resp)
        {
            Downloader::open(url, resp);
            boost::system::error_code ec;
            if (sink_->open(url, ec)) {
                response(ec);
                return;
            }
            dispatcher_->async_open(
                url, 
                boost::bind(&DispatchDownloader::handle_open, this ,_1));
        }

        bool DispatchDownloader::cancel(
            boost::system::error_code & ec)
        {
            return dispatcher_->cancel(ec);
        }

        bool DispatchDownloader::close(
            boost::system::error_code & ec)
        {
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
            ppbox::data::PlayInfo info;
            if (!dispatcher_->get_play_info(info, ec))
                return false;
            stat.total_size = info.byte_range.end;
            stat.finish_size = info.byte_range.pos;
            stat.speed = calc_speed(info.byte_range.pos);
            return true;
        }

        void DispatchDownloader::handle_open(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_open] ec:" << ec.message());

            boost::system::error_code ec1 = ec;
            if (!ec1) {
                dispatcher_->setup(-1, *sink_, ec1);
            }
            if (ec1) {
                response(ec1);
                return;
            }

            opened_ = true;

            ppbox::dispatch::SeekRange range;
            range.type = ppbox::dispatch::SeekRange::byte;
            sink_->file_stream().seekp(0, std::ios::end);
            range.beg = sink_->file_stream().tellp();
            if (range.beg == 0) {
                range.type = ppbox::dispatch::SeekRange::none;
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
