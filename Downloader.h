// Downloader.h

#ifndef _PPBOX_DOWNLOAD_DOWNLOADER_H_
#define _PPBOX_DOWNLOAD_DOWNLOADER_H_
#include "ppbox/download/PptvHttpHandle.h"
#include "ppbox/download/PptvHttpDownloader.h"

#include <string>
#include <fstream>

#include <boost/cstdint.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>

#include <framework/network/NetName.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/RefenceFromThis.h>

namespace ppbox
{
    namespace download
    {
        class Downloader
            : public util::RefenceFromThis<Downloader>
        {
        public:
            enum DownloadPart
            {
                HEADER,
                BODY,
            };

            enum StateEnum
            {
                NOTSTART,
                START,
                JUMP,
                DRAG,
                DOWNLOADHEAD,
                DOWNLOADBODY,
                FINISH,
                FAILED,
            };

        public:
            Downloader(boost::asio::io_service & io_srv);
            ~Downloader();

            boost::system::error_code start(char const * name,
                                            boost::system::error_code & ec);

            void stop(void);

            void set_file_name(char const * filename);

            void set_download_path(char const * download_path);

            boost::system::error_code get_last_error();

            boost::system::error_code get_download_statictis(DownloadStatistic & stat,
                                                             boost::system::error_code & ec);

            Downloader::StateEnum get_download_stat(void) const;

        private:
            void jump_callback(boost::system::error_code const & ec);

            void drag_callback(boost::system::error_code const & ec);

            void download_callback(boost::system::error_code const & ec);

            boost::system::error_code get_request(boost::uint64_t segment,
                framework::network::NetName & addr,
                util::protocol::HttpRequest & request,
                boost::system::error_code & ec);

            boost::system::error_code mp4_join(boost::system::error_code & ec);

            std::string get_key() const;

            // 断点续传检查
            bool check_resunme(boost::uint32_t & segment, boost::uint64_t & offset);

        private:
            boost::asio::io_service & io_srv_;
            PptvHttpHandle * http_handle_;
            PptvHttpDownloader * http_download_;
            boost::system::error_code last_error_;
            boost::uint16_t   ppap_port_;
            std::string name_;
            time_t local_time_;
            std::string head_file_name_;
            std::string part_file_name_;
            std::string end_file_name_;
            std::string download_path_;
            std::string file_name_;
            std::ofstream           ofs_;

            boost::uint32_t         current_segment_pos_;
            boost::uint32_t         total_segments_;
            DownloadPart            downlload_part_;

            bool  is_cancel_;
            StateEnum state_;
        };
    }
}

#endif /* _PPBOX_DOWNLOAD_DOWNLOADER_H_ */
