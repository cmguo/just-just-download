// Record.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/Record.h"

#include <ppbox/demux/DemuxerModule.h>
using namespace ppbox::demux;

using namespace boost::system;

namespace ppbox
{
    namespace download
    {
        error_code Record::create(
            error_code & ec)
        {
            record_demux_ = (RecordDemuxer *)DemuxerModule::find("record");
            if (!record_demux_) {
                ec = error::not_found_downloader;
            }
            return ec;
        }

        void Record::set_pool_size(boost::uint32_t size)
        {
            assert(record_demux_ != NULL);
            record_demux_->set_pool_size(size);
        }

        void Record::set_video_info(
            boost::uint32_t index,
            boost::uint32_t width,
            boost::uint32_t height,
            boost::uint32_t frame_rate,
            boost::uint32_t time_scale,
            std::vector<boost::uint8_t> const & spec_data)
        {
            assert(record_demux_ != NULL);
            MediaInfo info;
            info.type = MEDIA_TYPE_VIDE;
            info.sub_type = VIDEO_TYPE_AVC1;
            info.format_type = MediaInfo::video_avc_byte_stream;
            info.format_data.assign(spec_data.begin(), spec_data.end());
            info.time_scale = time_scale;
            info.video_format.bitrate = 0;
            info.video_format.frame_rate = frame_rate;
            info.video_format.height = height;
            info.video_format.width = width;
            record_demux_->add_stream(info);
        }

        void Record::set_audio_info(
            boost::uint32_t index,
            boost::uint32_t sample_rate,
            boost::uint32_t channel_count,
            boost::uint32_t sample_size,
            boost::uint32_t time_scale,
            std::vector<boost::uint8_t> const & spec_data)
        {
            assert(record_demux_ != NULL);
            MediaInfo info;
            info.type = MEDIA_TYPE_AUDI;
            info.sub_type = AUDIO_TYPE_MP4A;
            info.format_type = MediaInfo::audio_microsoft_wave;
            info.format_data.assign(spec_data.begin(), spec_data.end());
            info.time_scale = time_scale;
            info.audio_format.bitrate = 0;
            info.audio_format.channel_count = channel_count;
            info.audio_format.sample_rate = sample_rate;
            info.audio_format.sample_size = sample_size;
            record_demux_->add_stream(info);
        }

        void Record::push(ppbox::demux::Frame const & frame)
        {
            assert(record_demux_ != NULL);
            record_demux_->push(frame);
        }

        void Record::destory(void)
        {
            record_demux_ = NULL;
        }
    } // namespace download
} // namespace ppbox
