// Record.h

#ifndef _PPBOX_DOWNLOAD_RECORD_H_
#define _PPBOX_DOWNLOAD_RECORD_H_

#include <ppbox/demux/RecordDemuxer.h>

namespace ppbox
{
    namespace download
    {
        class Record
        {
        public:
            Record()
                : record_demux_(NULL)
            {
            }

            ~Record()
            {
                record_demux_ = NULL;
            }

            boost::system::error_code create(
                boost::system::error_code & ec);

            void set_pool_size(boost::uint32_t size);

            void set_audio_info(
                boost::uint32_t index,
                boost::uint32_t sample_rate,
                boost::uint32_t channel_count,
                boost::uint32_t sample_size,
                boost::uint32_t time_scale,
                std::vector<boost::uint8_t> const & spec_data);

            void set_video_info(
                boost::uint32_t index,
                boost::uint32_t width,
                boost::uint32_t height,
                boost::uint32_t frame_rate,
                boost::uint32_t time_scale,
                std::vector<boost::uint8_t> const & spec_data);

            void push(ppbox::demux::Frame const & frame);

            void destory(void);

        private:
            ppbox::demux::RecordDemuxer * record_demux_;
        };

    } // namespace download
} // namespace ppbox

#endif
