#ifndef      _PPBOX_DOWNLOAD_HTTPHANDLE_
#define      _PPBOX_DOWNLOAD_HTTPHANDLE_

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

#include <framework/network/NetName.h>

namespace util
{
    namespace protocol
    {
        class HttpClient;
    }
}

namespace ppbox
{
    namespace download
    {
        struct JumpServerInfo
        {
            time_t server_time;
            framework::network::NetName server_host;
            int  bwtype;
        };

        struct PptvMp4Segment
        {
            std::string     no;
            boost::uint32_t duration;
            boost::uint32_t duration_offset;
            std::string     va_rid;
            char            rid[36];
            boost::uint64_t file_length;
            boost::uint64_t head_length;
        };

        typedef std::vector<PptvMp4Segment>   Mp4Segments;

        struct PptvMp4Video
        {
            std::string name;
            std::string type;
            bool hasvideo;
            bool hasaudio;
            bool antisteal;
            boost::uint32_t bitrate;
            boost::uint32_t width;
            boost::uint32_t height;
            boost::uint32_t duration;
            boost::uint64_t filesize;
        };

        class PptvHttpHandle
        {
        public:
            PptvHttpHandle(boost::asio::io_service & io_srv);
            ~PptvHttpHandle();

            typedef boost::function<void (
                boost::system::error_code const &)> access_jump_response_type;

            void async_access_jump(
                const char * video_name,
                access_jump_response_type const & resp);

            typedef boost::function<void (
                boost::system::error_code const &)> access_drag_response_type;

            void async_access_drag(
                access_drag_response_type const & resp);

            JumpServerInfo & get_jump_info();

            PptvMp4Video & get_mp4_video_info();

            Mp4Segments & get_mp4_segment_info();

            void cancel(void);

        private:

            void handle_jump_fetch(boost::system::error_code const & ec);

            void handle_drag_fetch(boost::system::error_code const & ec);

            boost::system::error_code parse_jump(boost::system::error_code & ec);

            boost::system::error_code parse_drag(boost::system::error_code & ec);

        private:
            util::protocol::HttpClient * http_client_;
            access_jump_response_type      jump_resp_;
            access_drag_response_type      drag_resp_;
            std::string video_name_;
            JumpServerInfo jump_info_;
            PptvMp4Video  mp4_video_;
            Mp4Segments mp4_segments_;

            bool cancle_;
        };
    }
}

#endif  /* _PPBOX_DOWNLOAD_HTTPHANDLE_ */
