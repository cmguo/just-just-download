// PptvHttpDownloader.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/PptvHttpDownloader.h"

#include <boost/bind.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/error.hpp>
#include <boost/thread/thread.hpp>
using namespace boost::asio;
using namespace boost::asio::error;
using namespace boost::system;

#include <framework/network/NetName.h>
#include <framework/logger/LoggerStreamRecord.h>
#include <framework/string/Format.h>
#include <util/protocol/http/HttpClient.h>
#include <util/protocol/http/HttpFieldRange.h>
using namespace framework::network;
using namespace framework::logger;
using namespace framework::string;
using namespace util::protocol;

FRAMEWORK_LOGGER_DECLARE_MODULE("ppbox_download");

namespace ppbox
{
    namespace download
    {
        PptvHttpDownloader::PptvHttpDownloader(io_service & io_srv,
                                               std::ofstream & ofs)
            : http_client_(new HttpClient(io_srv))
            , cancel_(false)
            , download_size_(0)
            , receive_sum_size_(0)
            , leave_size_(-1)
            , state_(NOTSTART)
            , ofs_(ofs)
        {
        }

        PptvHttpDownloader::~PptvHttpDownloader()
        {
            if (http_client_) {
                delete http_client_;
                http_client_ = NULL;
            }
        }

        void PptvHttpDownloader::SetRange(boost::uint64_t begin, boost::uint64_t end)
        {
            range_.begin = begin;
            range_.end = end;
        }

        void PptvHttpDownloader::reset(void)
        {
            range_.reset();
            download_size_    = 0;
            receive_sum_size_ = 0;
            leave_size_       = -1;
            state_            = NOTSTART;
        }

        void PptvHttpDownloader::async_open(
                HttpRequest const & request,
                NetName const & addr,
                open_download_callback_type const & resp)
        {
            LOG_S(Logger::kLevelDebug, "ppbox download async open ... ");
            error_code ec;
            request_ = request;
            addr_ = addr;
            resp_ = resp;

            if (range_.begin != 0 || range_.end != (boost::uint64_t)-1) {
                request_.head().range.reset(http_filed::Range((boost::int64_t)range_.begin
                    ,(boost::int64_t)range_.end));
                download_size_ = range_.end - range_.begin;
                LOG_S(Logger::kLevelAlarm, "request head range: begin = " << range_.begin << ", end = " << range_.end);
            } else {
                request_.head().range.reset();
            }

            http_client_->bind_host(addr_, ec);
            if (!ec) {
                http_client_->set_non_block(true, ec);
            }

            if (ec) {
                last_error_ = ec;
                resp_(last_error_);
                resp_.clear();
            } else {
                http_client_->async_open(request_, boost::bind(&PptvHttpDownloader::http_open_callback, this, _1));
            }
        }

        void PptvHttpDownloader::http_open_callback(error_code const & ec)
        {
            if (cancel_) {
                last_error_ = error::download_canceled;
                state_ = CANCEL;
                open_download_callback_type resp;
                resp.swap(resp_);
                resp(last_error_);
                return;
            }

            if (!ec) {
                reset();
                http_client_->get_response_head().get_content(std::cout);
                boost::asio::async_read(*http_client_,
                    boost::asio::buffer(buf_),
                    boost::asio::transfer_all(),
                    boost::bind(&PptvHttpDownloader::download_handler, this, _1, _2));

            } else {
                std::cout << "open download url error, ec = " << ec.message() << std::endl;
                state_ = FAILED;
                last_error_ = error::open_download_link_failed;
                resp_(last_error_);
                resp_.clear();
            }
        }

        void PptvHttpDownloader::cancel(void)
        {
            cancel_ = true;
            http_client_->cancel();
        }

        void PptvHttpDownloader::close(void)
        {
            this->cancel();
        }

        DownloadState PptvHttpDownloader::GetState(void) const
        {
            return state_;
        }

        void PptvHttpDownloader::download_handler(error_code const & ec,
                                                  std::size_t bytes_transferred)
        {
            if (cancel_) {
                last_error_ = error::download_canceled;
                state_ = CANCEL;
                resp_(last_error_);
                resp_.clear();
                return;
            }
            if (ec) {
                if (bytes_transferred > 0) {
                    // write statictis
                    add_finish_size(bytes_transferred);

                    ofs_.write(buf_.data(), bytes_transferred);
                    receive_sum_size_ += bytes_transferred;
                    std::cout << "download completed" << std::endl;
                    state_ = COMPLETE;
                    error_code lec;
                    resp_(lec);
                    resp_.clear();
                } else {
                    last_error_ = error::download_network_error;
                    state_ = UNCOMPLETE;
                    resp_(last_error_);
                    resp_.clear();
                }
            } else {
                if (bytes_transferred > 0) {
                    // write statictis
                    add_finish_size(bytes_transferred);

                    ofs_.write(buf_.data(), bytes_transferred);
                    receive_sum_size_ += bytes_transferred;
                    leave_size_ = download_size_ - receive_sum_size_;
                    if (receive_sum_size_ == download_size_) {
                        std::cout << "download completed" << std::endl;
                        state_ = COMPLETE;
                        error_code lec;
                        resp_(lec);
                        resp_.clear();
                    } else if (receive_sum_size_ < download_size_ ) {
                        reset();
                        if (leave_size_ < buf_.size()) {
                            boost::asio::async_read(*http_client_,
                                boost::asio::buffer(buf_, (size_t)leave_size_),
                                boost::asio::transfer_all(),
                                boost::bind(&PptvHttpDownloader::download_handler, this, _1, _2));
                        } else {
                            boost::asio::async_read(*http_client_,
                                boost::asio::buffer(buf_),
                                boost::asio::transfer_all(),
                                boost::bind(&PptvHttpDownloader::download_handler, this, _1, _2));
                        }
                    } else {
                        last_error_ = error::download_data_overflow;
                        state_ = FAILED;
                        resp_(last_error_);
                        resp_.clear();
                    }
                }
            }
        }
    } /* namespace download */
} /* namespace ppbox */


