#include "ppbox/download/Common.h"
#include "ppbox/download/PptvHttpHandle.h"

#include <boost/bind.hpp>
using namespace boost::asio;
using namespace boost::system;

#include <framework/network/NetName.h>
#include <framework/logger/LoggerStreamRecord.h>
#include <framework/string/Format.h>
#include <util/protocol/http/HttpClient.h>
using namespace framework::network;
using namespace framework::logger;
using namespace framework::string;
using namespace util::protocol;

#include <tinyxml/tinyxml.h>

FRAMEWORK_LOGGER_DECLARE_MODULE("ppbox_download");

#ifndef PPBOX_DNS_DOWNLOAD_JUMP
#define PPBOX_DNS_DOWNLOAD_JUMP "(tcp)(v4)j.150hi.com:80"
#endif

#ifndef PPBOX_DNS_DOWNLOAD_DRAG
#define PPBOX_DNS_DOWNLOAD_DRAG "(tcp)(v4)d.150hi.com:80"
#endif

namespace ppbox
{
    namespace download
    {
        static const NetName dns_jump_server(PPBOX_DNS_DOWNLOAD_JUMP);
        static const NetName dns_drag_server(PPBOX_DNS_DOWNLOAD_DRAG);

        static time_t UTCtoLocaltime(
            std::string const & utctime)
        {
            static std::map<std::string, int> week1;
            static std::map<std::string, int> months1;
            if (week1.empty()) {
                week1["Sun"] = 0;
                week1["Mon"] = 1;
                week1["Tue"] = 2;
                week1["Wed"] = 3;
                week1["Thu"] = 4;
                week1["Fri"] = 5;
                week1["Sat"] = 6;

                months1["Jan"] = 0;
                months1["Feb"] = 1;
                months1["Mar"] = 2;
                months1["Apr"] = 3;
                months1["May"] = 4;
                months1["Jun"] = 5;
                months1["Jul"] = 6;
                months1["Aug"] = 7;
                months1["Sep"] = 8;
                months1["Oct"] = 9;
                months1["Nov"] = 10;
                months1["Dec"] = 11;
            }

            time_t rr_time;
            std::vector<std::string> Res;
            slice<std::string>(utctime, std::inserter(Res, Res.end()), " ");
            std::vector<int> Res1;
            slice<int>(Res[3], std::inserter(Res1, Res1.end()), ":");
            tm m_tm;
            m_tm.tm_year   = atoi(Res[4].c_str())-1900;
            m_tm.tm_mon    = months1[Res[1]];
            m_tm.tm_yday   = 0;
            m_tm.tm_mday   = atoi(Res[2].c_str());
            m_tm.tm_wday   = week1[Res[0]];
            m_tm.tm_hour   = Res1[0];
            m_tm.tm_min    = Res1[1];
            m_tm.tm_sec    = Res1[2];
            m_tm.tm_isdst  = -1;
            rr_time = mktime(&m_tm) - timezone;
            return rr_time;
        }

        static std::string addr_host(
            NetName const & addr)
        {
            return addr.host() + ":" + addr.svc();
        }

        PptvHttpHandle::PptvHttpHandle(io_service & io_srv)
                       : http_client_(new HttpClient(io_srv))
                       , cancle_(false)
        {
        }

        PptvHttpHandle::~PptvHttpHandle()
        {
            if (http_client_) {
                delete http_client_;
                http_client_ = NULL;
            }
        }

        void PptvHttpHandle::async_access_jump(const char * video_name
                                               ,access_jump_response_type const & resp)
        {
            LOG_S(Logger::kLevelDebug, "in async_access_jump funcion");

            video_name_ = video_name;
            error_code ec;
            http_client_->bind_host(dns_jump_server, ec);

            if (ec) {
                resp(ec);
                return;
            }

            jump_resp_ = resp;

            HttpRequestHead head;
            head.method = HttpRequestHead::get;
            head.path = "/" + video_name_ + "dt?t=" + format(rand()) + "&type=ppbox";
            LOG_S(Logger::kLevelDebug, "head path: " << head.path);
            head.host = dns_jump_server.host();

            http_client_->async_fetch(head,
                boost::bind(&PptvHttpHandle::handle_jump_fetch, this, _1));
        }

        void PptvHttpHandle::handle_jump_fetch(error_code const & ec)
        {
            if (cancle_) {
                jump_resp_(error::download_canceled);
                jump_resp_.clear();
                return;
            }

            LOG_S(Logger::kLevelDebug, "in handle_jump_fetch funcion: ec " << ec.message());
            error_code lec = ec;
            if (!lec) {
                parse_jump(lec);
            }

            // 关闭与jump服务器的连接
            error_code lec1;
            http_client_->close(lec1);
            jump_resp_(lec);
            jump_resp_.clear();
        }

        error_code PptvHttpHandle::parse_jump(error_code & ec)
        {
            ec = error_code();
            TiXmlDocument xmlDoc;
            {
                *buffer_cast<char *>(http_client_->response().data().prepare(1)) = '\0';
                http_client_->response().data().commit(1);
                std::string buffer = buffer_cast<char const *>(http_client_->response().data().data());
                xmlDoc.Parse(buffer.c_str());
            }
            if (xmlDoc.Error()) {
                return ec = error::bad_jump_file_format;
            }

            TiXmlElement * root = xmlDoc.RootElement();
            if (!root) {
                return ec = error::bad_jump_file_format;
            }
            TiXmlElement * xml_server_host = root->FirstChildElement("server_host");
            TiXmlElement * xml_server_time = root->FirstChildElement("server_time");
            TiXmlElement * xml_BWType = root->FirstChildElement("BWType");
            if (!xml_server_host || !xml_server_time || !xml_BWType) {
                return ec = error::bad_jump_file_format;
            }
            char const * pText = NULL;
            if ((pText = xml_server_host->GetText())) {
                jump_info_.server_host.family(NetName::v4);
                jump_info_.server_host.svc("80");
                jump_info_.server_host.from_string(pText);
            }
            if ((pText = xml_server_time->GetText())) {
                jump_info_.server_time = UTCtoLocaltime(pText);
            }
            if ((pText = xml_BWType->GetText())) {
                jump_info_.bwtype = atoi(pText);
            }
            return ec;
        }

        void PptvHttpHandle::async_access_drag(
            access_drag_response_type const & resp)
        {
            LOG_S(Logger::kLevelDebug, "in async_access_drag funcion");
            error_code ec;
            http_client_->bind_host(dns_drag_server, ec);

            if (ec) {
                resp(ec);
                return;
            }

            drag_resp_ = resp;

            HttpRequestHead head;
            head.path = "/" + video_name_ + "0drag";
            LOG_S(Logger::kLevelDebug, "head path: " << head.path);
            head.host.reset(addr_host(dns_drag_server));

            http_client_->async_fetch(head,
                boost::bind(&PptvHttpHandle::handle_drag_fetch, this, _1));
        }

        void PptvHttpHandle::handle_drag_fetch(error_code const & ec)
        {
            LOG_S(Logger::kLevelDebug, "in handle_drag_fetch funcion: ec " << ec.message());
            if (cancle_) {
                drag_resp_(error::download_canceled);
                drag_resp_.clear();
                return;
            }

            error_code lec = ec;
            if (!lec) {
                parse_drag(lec);
            }

            // 关闭与drag服务器的连接
            error_code lec1;
            http_client_->close(lec1);
            drag_resp_(lec);
            drag_resp_.clear();
        }

        JumpServerInfo & PptvHttpHandle::get_jump_info()
        {
            return jump_info_;
        }

        PptvMp4Video & PptvHttpHandle::get_mp4_video_info()
        {
            return mp4_video_;
        }

        Mp4Segments & PptvHttpHandle::get_mp4_segment_info()
        {
            return mp4_segments_;
        }

        void PptvHttpHandle::cancel(void)
        {
            cancle_ = true;
            if (http_client_) {
                http_client_->cancel();
            }
        }

        error_code PptvHttpHandle::parse_drag(
                   error_code & ec)
        {
            ec = error_code();
            TiXmlDocument xmlDoc;
            {
                *buffer_cast<char *>(http_client_->response().data().prepare(1)) = '\0';
                http_client_->response().data().commit(1);
                std::string buffer = buffer_cast<char const *>(http_client_->response().data().data());
                xmlDoc.Parse(buffer.c_str());
            }
            if (xmlDoc.Error()) {
                return ec = error::bad_drag_file_format;
            }

            TiXmlElement * xml_root = xmlDoc.RootElement();
            if (!xml_root) {
                return ec = error::bad_drag_file_format;
            }
            TiXmlElement * xml_video = xml_root->FirstChildElement("video");
            TiXmlElement * xml_segments = xml_root->FirstChildElement("segments");
            if (!xml_segments || !xml_video) {
                return ec = error::bad_drag_file_format;
            }

            {
                char const * pText = NULL;
                if ((pText = xml_video->Attribute("name"))) {
                    mp4_video_.name = pText;
                } else {
                    return ec = error::bad_drag_file_format;
                }
                TiXmlElement * element = NULL;
                element = xml_video->FirstChildElement("type");
                if (element && (pText = element->GetText())) {
                    mp4_video_.type = pText;
                }
                element = xml_video->FirstChildElement("hasvideo");
                if (element && (pText = element->GetText())) {
                    mp4_video_.hasvideo = strcmp(pText, "true") == 0;
                }
                element = xml_video->FirstChildElement("hasaudio");
                if (element && (pText = element->GetText())) {
                    mp4_video_.hasaudio = strcmp(pText, "true") == 0;
                }
                element = xml_video->FirstChildElement("bitrate");
                if (element && (pText = element->GetText())) {
                    mp4_video_.bitrate = atoi(pText);
                }
                element = xml_video->FirstChildElement("filesize");
                if (element && (pText = element->GetText())) {
                    parse2(pText, mp4_video_.filesize);
                }
                element = xml_video->FirstChildElement("duration");
                if (element && (pText = element->GetText())) {
                    mp4_video_.duration = (boost::uint32_t)(atof(pText) * 1000.0f);
                }
                element = xml_video->FirstChildElement("antisteal");
                if (element && (pText = element->GetText())) {
                    mp4_video_.antisteal = strcmp(pText, "true") == 0;
                }
                element = xml_video->FirstChildElement("width");
                if (element && (pText = element->GetText())) {
                    mp4_video_.width = atoi(pText);
                }
                element = xml_video->FirstChildElement("height");
                if (element && (pText = element->GetText())) {
                    mp4_video_.height = atoi(pText);
                }
            }

            TiXmlElement * xml_segment = xml_segments->FirstChildElement();
            boost::uint64_t duration_offset = 0;
            boost::uint64_t file_length = 0;
            while(xml_segment) {
                if (strcmp(xml_segment->Value(), "segment") == 0) {
                    PptvMp4Segment seg;
                    char const * pText = NULL;
                    if ((pText = xml_segment->Attribute("no"))) {
                        seg.no = pText;
                    }
                    if ((pText = xml_segment->Attribute("duration"))) {
                        seg.duration = (boost::uint32_t)(atof(pText) * 1000.0f);
                    }
                    if ((pText = xml_segment->Attribute("varid"))) {
                        seg.va_rid = pText;

                        std::vector<std::string> cmd_args;
                        slice<std::string>(seg.va_rid, std::inserter(cmd_args, cmd_args.end()), "&");
                        if (strncmp(cmd_args[0].c_str(), "rid=", 4) == 0) {
                            memcpy(seg.rid, cmd_args[0].c_str() + 4, 32);
                            seg.rid[32] = '\0';
                        }
                    }
                    if ((pText = xml_segment->Attribute("filesize"))) {
                        parse2(pText, seg.file_length);
                        file_length += seg.file_length;
                    }
                    if ((pText = xml_segment->Attribute("headlength"))) {
                        parse2(pText, seg.head_length);
                    }
                    if (!seg.no.empty() && seg.duration > 0 && !seg.va_rid.empty()) {
                        seg.duration_offset = duration_offset;
                        duration_offset += seg.duration;
                        mp4_segments_.push_back(seg);
                    } else {
                        ec = error::bad_drag_file_format;
                        break;
                    }
                }
                xml_segment = xml_segment->NextSiblingElement();
            }
            return ec;
        }
    }
}


