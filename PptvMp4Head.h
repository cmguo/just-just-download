// PptvMp4Head.h:

#ifndef _PPBOX_DOWNLAOD_MP4HEADER_
#define _PPBOX_DOWNLAOD_MP4HEADER_

#include <bento4/Core/Ap4Atom.h>
#include <bento4/Core/Ap4TkhdAtom.h>
#include <bento4/Core/Ap4SttsAtom.h>
#include <bento4/Core/Ap4CttsAtom.h>
#include <bento4/Core/Ap4StscAtom.h>
#include <bento4/Core/Ap4StszAtom.h>
#include <bento4/Core/Ap4StcoAtom.h>
#include <bento4/Core/Ap4StssAtom.h>
#include <bento4/Core/Ap4StsdAtom.h>
#include <bento4/Core/Ap4Co64Atom.h>
#include <bento4/Core/Ap4ByteStream.h>
#include <bento4/Core/Ap4PptvFileByteStream.h>
#include <bento4/Core/Ap4FileByteStream.h>
#include <bento4/Core/Ap4File.h>
#include <bento4/Core/Ap4Movie.h>
#include <bento4/Core/Ap4Sample.h>
#include <bento4/Core/Ap4SampleDescription.h>
#include <bento4/Core/Ap4Track.h>
#include <bento4/Core/Ap4Protection.h>
#include <bento4/Core/Ap4AvccAtom.h>
#include <bento4/Core/Ap4TrakAtom.h>
#include <bento4/Core/Ap4StsdAtom.h>
#include <bento4/Core/Ap4HdlrAtom.h>
#include <bento4/Core/Ap4DataBuffer.h>
#include <bento4/Core/Ap4FtypAtom.h>
#include <bento4/Core/Ap4FileWriter.h>
//#include <bento4/Core/Ap4PPMdatAtom.h>

namespace ppbox
{
    namespace download
    {
        class PptvMp4Head
        {
        public:
            PptvMp4Head();
            virtual ~PptvMp4Head();

        public:
            boost::system::error_code CheckMp4Head(boost::system::error_code & ec);
            boost::system::error_code WriteToMemStream(char const * str, boost::int32_t n);
            boost::system::error_code WriteHeadToMemStream(boost::system::error_code & ec);
            boost::system::error_code ReadFromHeadStream(void * p, AP4_Size size);
            boost::system::error_code SeekHeadStream(AP4_Position position);
            boost::system::error_code CompareStsdDescription(AP4_Track * cur_track,
                                                             AP4_Track * source_track);
            AP4_Atom* FindChild(const char* path);
            boost::system::error_code FindTrack(PptvMp4Head &mp4_head,
                                                AP4_Track *cur_track,
                                                AP4_Track **p);

            AP4_Atom* FindTrackChild(AP4_Track * track, char const * path);
            boost::system::error_code MergeTrack(PptvMp4Head &mp4_head,
                                                 AP4_Track *cur_track,
                                                 AP4_Track *source_track,
                                                 AP4_StcoAtom **pp_cur_stco);

            boost::system::error_code Merge(PptvMp4Head &mp4_head);
            AP4_LargeSize GetHeadStreamSize();
            boost::uint64_t GetContentSize();
            boost::uint64_t GetContentLength();
            boost::uint64_t GetHeadSize();
            AP4_Size CaclHeadSize();
            void SetContentLength(AP4_Size content_length);
            void SetMemStreamEndPosition(AP4_Position pos);

        public:
            AP4_File       * file_;
            AP4_Movie      * movie_;
            AP4_FtypAtom   * ftyp_atom_; 
            // AP4_PPMdatAtom * mdat_;
            AP4_Atom       * mdat_; 
            //AP4_MemoryByteStream * stream_;
            AP4_ByteStream  * stream_;
            AP4_MemoryByteStream * head_stream_;
            bool            check_head_is_ok_;
            boost::uint64_t head_size_;
            AP4_Size        mdat_head_size_;
            boost::uint64_t content_length_;
            boost::uint64_t content_size_;
            AP4_Position    end_;

        public:
            void Clean();

        };
    }
}

#endif /* _PPBOX_DOWNLAOD_MP4HEADER_ */
