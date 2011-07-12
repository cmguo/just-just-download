// Downloader.cpp
#include "ppbox/download/Common.h"
#include "ppbox/download/Downloader.h"
#include "ppbox/download/PptvMp4Head.h"

#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
using namespace boost::system;
using namespace boost::filesystem;

#include <framework/logger/LoggerStreamRecord.h>
#include <framework/string/Url.h>
#include <framework/memory/MemoryPage.h>
#include <util/protocol/pptv/TimeKey.h>
using namespace framework::logger;
using namespace framework::string;
using namespace framework::network;
using namespace framework::memory;
using namespace util::protocol;

FRAMEWORK_LOGGER_DECLARE_MODULE("ppbox_download");

namespace ppbox
{
    namespace download
    {
        static std::string addr_host(
            NetName const & addr)
        {
            return addr.host() + ":" + addr.svc();
        }

        Downloader::Downloader(boost::asio::io_service & io_srv)
            : io_srv_(io_srv)
            , http_handle_(new PptvHttpHandle(io_srv_))
            , http_download_(NULL)
            , ppap_port_(9000)
            , downlload_part_(HEADER)
            , is_cancel_(false)
            , state_(NOTSTART)
        {
            LOG_S(Logger::kLevelAlarm, "Downloader constructor");
        }

        Downloader::~Downloader()
        {
            LOG_S(Logger::kLevelAlarm, "Downloader destory");
            if (http_handle_) {
                delete http_handle_;
                http_handle_ = NULL;
            }

            if (http_download_) {
                delete http_download_;
                http_download_ = NULL;
            }
        }

        error_code Downloader::start(char const * name, error_code & ec)

        {
            LOG_S(Logger::kLevelDebug, "Manager start ...");
            assert(http_handle_ != NULL);
            state_ = JUMP;
            name_ = name;
            local_time_ = time(NULL);
            http_handle_->async_access_jump(name, boost::bind(&Downloader::jump_callback, ref(this), _1));
            return ec;
        }

        void Downloader::stop(void)
        {
            is_cancel_ = true;
            if (http_handle_) {
                http_handle_->cancel();
            }

            if (http_download_) {
                http_download_->close();
            }
        }

        void Downloader::jump_callback(error_code const & ec)
        {
            LOG_S(Logger::kLevelDebug, "Jump callback, ec = " << ec.message());
            if (!ec) {
                state_ = DRAG;
                http_handle_->async_access_drag(boost::bind(&Downloader::drag_callback, ref(this), _1));
            } else {
                state_ = FAILED;
                last_error_ = ec;
            }
        }

        void Downloader::drag_callback(error_code const & ec)
        {
            LOG_S(Logger::kLevelDebug, "drag callback, ec = " << ec.message());
            error_code lec;
            if (!ec) {
                HttpRequest request;
                NetName addr;

                // set file name
                if (file_name_.empty()) {
                    file_name_ = name_;
                }
                head_file_name_ = file_name_ + ".tmp";
                head_file_name_ = download_path_ + head_file_name_;
                part_file_name_ = file_name_ + ".filepart";
                part_file_name_ = download_path_ + part_file_name_;
                end_file_name_  = download_path_ + file_name_;
                total_segments_      = http_handle_->get_mp4_segment_info().size();

                boost::uint64_t total_data_size = 0;
                boost::uint64_t total_head_size = 0;
                for(boost::uint32_t i = 0; i < total_segments_; ++i) {
                    total_data_size += http_handle_->get_mp4_segment_info()[i].file_length;
                    total_head_size += http_handle_->get_mp4_segment_info()[i].head_length;
                }
                // 检查是否已经下载完成过
                if (exists(end_file_name_)) {
                    last_error_ = error::download_finished_task;
                    state_ = FAILED;
                    return;
                }
                // 检查断点续传
                boost::uint64_t offset = 0;
                if (check_resunme(current_segment_pos_, offset)) {
                    if (current_segment_pos_ == (total_segments_-1)
                        && 0 ==offset
                        && exists(part_file_name_)) {
                        rename(part_file_name_, end_file_name_);
                        state_ = FINISH;
                        LOG_S(Logger::kLevelAlarm, "Download all file completed");
                    } else {
                        ofs_.open(part_file_name_.c_str(), std::ios::binary | std::ios::out | std::ios::app);
                        downlload_part_ = BODY;
                        get_request(current_segment_pos_, addr, request, lec);
                        if (!lec) {
                            http_download_ = new PptvHttpDownloader(io_srv_,  ofs_);
                            http_download_->begin_statictis();
                            http_download_->set_total_size(total_data_size);
                            http_download_->set_finish_size(total_head_size + offset);

                            if (HEADER == downlload_part_) {
                                http_download_->SetRange(offset, http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length);
                            } else if (BODY == downlload_part_) {
                                http_download_->SetRange(offset,
                                    http_handle_->get_mp4_segment_info()[current_segment_pos_].file_length);
                            } else {
                                // Nothing
                            }
                            state_ = DOWNLOADBODY;
                            http_download_->async_open(
                                request,
                                addr,
                                boost::bind(&Downloader::download_callback, ref(this), _1));
                        } else {
                            state_ = FAILED;
                            last_error_ = lec;
                        }
                    }

                } else {
                    current_segment_pos_ = 0;
                    ofs_.open(head_file_name_.c_str(), std::ios::binary | std::ios::out);
                    http_download_ = new PptvHttpDownloader(io_srv_,  ofs_);
                    http_download_->begin_statictis();
                    http_download_->set_total_size(total_data_size);

                    if (0 == total_segments_) {
                        last_error_ = error::not_found_any_segment;
                        return;
                    }

                    get_request(current_segment_pos_, addr, request, lec);
                    if (!lec) {
                        if (HEADER == downlload_part_) {
                            http_download_->SetRange(0, http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length);
                        } else if (BODY == downlload_part_) {
                            http_download_->SetRange(http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length,
                                http_handle_->get_mp4_segment_info()[current_segment_pos_].file_length);
                        } else {
                            // Nothing
                        }

                        state_ = DOWNLOADHEAD;
                        http_download_->async_open(
                            request,
                            addr,
                            boost::bind(&Downloader::download_callback, ref(this), _1));
                    } else {
                        last_error_ = lec;
                    }
                }
            } else {
                state_ = FAILED;
                last_error_ = ec;
            }
        }

        void Downloader::download_callback(error_code const & ec)
        {
            // 重新初始化下载模块
            http_download_->reset();
            http_download_->close();

            if (!ec) {
                if (current_segment_pos_ == (total_segments_ - 1)) {
                    std::cout << "download finished" << std::endl;
                    ofs_.close();
                    error_code lec;
                    if (downlload_part_ == HEADER) {
                        mp4_join(lec);
                        last_error_ = lec;
                    } else {
                        if (exists(part_file_name_)) {
                            rename(part_file_name_, end_file_name_);
                            state_ = FINISH;
                            LOG_S(Logger::kLevelAlarm, "Download all file completed");
                        } else {
                            state_ = FAILED;
                            LOG_S(Logger::kLevelAlarm, "Download all file failed");
                        }
                        http_download_->end_statictis();
                    }
                    
                } else {
                    HttpRequest request;
                    NetName addr;
                    error_code lec;

                    get_request(++current_segment_pos_, addr, request, lec);
                    if (!lec) {
                        if (HEADER == downlload_part_) {
                            http_download_->SetRange(0, http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length);
                        } else if (BODY == downlload_part_) {
                            http_download_->SetRange(http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length,
                                                     http_handle_->get_mp4_segment_info()[current_segment_pos_].file_length);
                        } else {
                            // Nothing
                        }
                        http_download_->async_open(
                            request,
                            addr,
                            boost::bind(&Downloader::download_callback, ref(this), _1));
                    } else {
                        state_ = FAILED;
                        last_error_ = lec;
                    }
                }
            } else {
                last_error_ = ec;
                state_ = FAILED;
                ofs_.close();
                http_download_->end_statictis();
            }
        }

        error_code Downloader::get_request(
                                        boost::uint64_t segment,
                                        NetName & addr,
                                        HttpRequest & request,
                                        error_code & ec)
        {
            HttpRequestHead & head = request.head();
            Url real_url("http://localhost/");
            ec = error_code();

            if (ppap_port_) {
                addr.host("127.0.0.1");
                addr.port(ppap_port_);
                head.host.reset(addr_host(addr));
            }

            real_url.host(http_handle_->get_jump_info().server_host.host());
            real_url.svc(http_handle_->get_jump_info().server_host.svc());
            real_url.path("/" + format(segment) + "/" + name_);
            real_url.param("key", get_key());
            if (ppap_port_) {
                Url url("http://localhost/ppvaplaybyopen?" + http_handle_->get_mp4_segment_info()[segment].va_rid);
                url.param("url", real_url.to_string());
                url.param("headlength", format(http_handle_->get_mp4_segment_info()[segment].head_length));
                url.param("autoclose", "false");
                url.param("drag", "1");
                url.param("BWType", format(http_handle_->get_jump_info().bwtype));

                std::string tmp =  url.to_string();
                url.encode();
                head.path = url.path_all();
            } else {
                head.path = real_url.path_all();
            }
            std::cout << "Segment url: " << real_url.to_string() << std::endl;

            return ec;
        }

        std::string Downloader::get_key() const
        {
            return pptv::gen_key_from_time(http_handle_->get_jump_info().server_time + (time(NULL) - local_time_));
        }

        error_code Downloader::get_last_error()
        {
            if (last_error_) {
                LOG_S(Logger::kLevelDebug, "last error, value = " << last_error_.value() << ", message = " << last_error_.message());
            }
            return last_error_;
        }

        bool Downloader::check_resunme(boost::uint32_t & segment, boost::uint64_t & offset)
        {
            bool res = false;
            if (exists(part_file_name_)) {
                boost::uint64_t filesize = file_size(part_file_name_);
                PptvMp4Head mp4_head;
                AP4_Result result = AP4_FileByteStream::Create(part_file_name_.c_str(),
                                                               AP4_FileByteStream::STREAM_MODE_READ,
                                                               mp4_head.stream_);
                if (AP4_FAILED(result)) {
                    LOG_S(Logger::kLevelAlarm, "parse partfile failed");
                } else {
                    error_code lec;
                    mp4_head.CheckMp4Head(lec);
                    if (lec) {
                        last_error_ = lec;
                    } else {
                        boost::uint64_t headsize = mp4_head.CaclHeadSize();
                        boost::int64_t remainsize = filesize - headsize;
                        for (boost::uint32_t i = 0; i < total_segments_; ++i) {
                            boost::uint64_t bodysize = http_handle_->get_mp4_segment_info()[i].file_length
                                - http_handle_->get_mp4_segment_info()[i].head_length;
                            if (remainsize == bodysize) {
                                segment = i + 1;
                                offset = 0;
                                res = true;
                                break;
                            } else if (remainsize < bodysize) {
                                segment = i;
                                offset = remainsize + http_handle_->get_mp4_segment_info()[i].head_length;
                                res = true;
                                break;
                            }
                            remainsize -= bodysize;
                        }
                    }
                }
            }
            return res;
        }

        error_code Downloader::mp4_join(error_code & ec)
        {
            if (1 == total_segments_) {
                return ec;
            }

            error_code lec;
            boost::uint64_t offset = 0;
            AP4_ByteStream* obj_stream = NULL;
            AP4_Result result = AP4_PptvFileByteStream::Create(head_file_name_.c_str(),
                                                           AP4_PptvFileByteStream::STREAM_MODE_READ,
                                                           obj_stream,
                                                           offset,
                                                           http_handle_->get_mp4_segment_info()[0].head_length);
            if (AP4_FAILED(result)) {
                std::cout << "ERROR: cannot open input file: " << head_file_name_.c_str() << std::endl;
                ec = error::join_mp4_segment_failed;
                return ec;
            }

            {
                PptvMp4Head obj_mp4_head;
                obj_mp4_head.stream_ = obj_stream;
                obj_mp4_head.CheckMp4Head(lec);
                if (lec) {
                    return ec = lec;
                }
                obj_mp4_head.SetContentLength(http_handle_->get_mp4_segment_info()[0].file_length);
                offset = http_handle_->get_mp4_segment_info()[0].head_length;
                boost::uint64_t segment_head_size = 0;
                for (boost::uint32_t i = 1; i < total_segments_; ++i) {
                    segment_head_size = http_handle_->get_mp4_segment_info()[i].head_length;
                    PptvMp4Head segment_mp4_head;
                    result = AP4_PptvFileByteStream::Create(head_file_name_.c_str(),
                                                            AP4_PptvFileByteStream::STREAM_MODE_READ,
                                                            segment_mp4_head.stream_,
                                                            offset,
                                                            http_handle_->get_mp4_segment_info()[i].head_length);

                    if (AP4_FAILED(result)) {
                        std::cout << "ERROR: cannot open input file: " << head_file_name_.c_str() << std::endl;
                        ec = error::join_mp4_segment_failed;
                        break;
                    }

                    segment_mp4_head.CheckMp4Head(lec);
                    if (lec) {
                        ec = lec;
                        break;
                    }
                    segment_mp4_head.SetContentLength(http_handle_->get_mp4_segment_info()[i].file_length);

                    lec = obj_mp4_head.Merge(segment_mp4_head);
                    if (lec) {
                        ec = lec;
                        break;
                    }
                    offset += http_handle_->get_mp4_segment_info()[i].head_length;
                }

                // 写文件
                if (!lec) {
                    AP4_ByteStream* output = NULL;
                    // refine rename  use tmp name
                    result = AP4_FileByteStream::Create(part_file_name_.c_str(),
                                                        AP4_FileByteStream::STREAM_MODE_WRITE,
                                                        output);
                    if (AP4_FAILED(result)) {
                        std::cout << "ERROR: cannot open output file: " << part_file_name_ << std::endl;
                        ec = error::join_mp4_segment_failed;
                    } else {
                        result = AP4_FileWriter::PptvWrite(*obj_mp4_head.file_,
                                                           *output,
                                                           obj_mp4_head.content_length_
                                                              - obj_mp4_head.head_size_
                                                              + obj_mp4_head.mdat_head_size_);
                        if (AP4_FAILED(result)) {
                            ec = error::invalid_mp4_head;
                        }
                    }
                    if (output) {
                        output->Release();
                        output = NULL;
                    }
                }
            }

            if (exists(head_file_name_)) {
                remove(head_file_name_);
            }

            // begin download data
            if (!ec) {
                HttpRequest request;
                NetName addr;
                ofs_.open(part_file_name_.c_str(), std::ios::binary | std::ios::out | std::ios::app);
                current_segment_pos_ = 0;
                downlload_part_ = BODY;

                get_request(current_segment_pos_, addr, request, lec);
                if (!lec) {
                    if (HEADER == downlload_part_) {
                        http_download_->SetRange(0, http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length);
                    } else if (BODY == downlload_part_) {
                        http_download_->SetRange(http_handle_->get_mp4_segment_info()[current_segment_pos_].head_length,
                            http_handle_->get_mp4_segment_info()[current_segment_pos_].file_length);
                    } else {
                        // Nothing
                    }
                    state_ = DOWNLOADBODY;
                    http_download_->async_open(
                        request,
                        addr,
                        boost::bind(&Downloader::download_callback, ref(this), _1));
                }
            }

            return ec;
        }

        void Downloader::set_download_path(char const * download_path)
        {
            download_path_ = download_path;
        }

        void Downloader::set_file_name(char const * filename)
        {
            file_name_ = filename;
        }

        error_code Downloader::get_download_statictis(DownloadStatistic & stat,
                                          error_code & ec)
        {
            if (http_download_) {
                stat.begin_time = http_download_->get_download_stat().begin_time;
                stat.finish_size = http_download_->get_download_stat().finish_size;
                stat.speed = http_download_->get_download_stat().speed;
                stat.total_size = http_download_->get_download_stat().total_size;
            } else {
                ec = error::not_start_download_data;
            }
            return ec;
        }

        Downloader::StateEnum Downloader::get_download_stat(void) const
        {
            return state_;
        }

    } /* namespace download */
} /* namespace ppbox */
