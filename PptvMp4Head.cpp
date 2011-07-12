// PptvMp4Head.cpp

#include "ppbox/download/Common.h"
#include "ppbox/download/PptvMp4Head.h"

#include <iostream>
#include <vector>

using namespace boost::system;

namespace ppbox
{
    namespace download
    {
        PptvMp4Head::PptvMp4Head()
        {
            file_ = NULL;
            movie_ = NULL;
            ftyp_atom_ = NULL;
            mdat_ = NULL;
            check_head_is_ok_ = false;
            stream_ = NULL;
            head_stream_ = NULL;
            head_size_ = 0;
            content_length_ = 0;
            content_size_ = 0;
            mdat_head_size_ = 0;
            end_ = 0;
        }

        PptvMp4Head::~PptvMp4Head()
        {
            Clean();
            if (stream_)
            {
                stream_->Release();
                stream_ = NULL;
            }
            if (head_stream_)
            {
                head_stream_->Release();
                head_stream_ = NULL;
            }
        }

        error_code PptvMp4Head::CheckMp4Head(error_code & ec)
        {
            try
            {
                file_ = new AP4_File(*stream_, AP4_DefaultAtomFactory::Instance);
                if (file_)
                {
                    ftyp_atom_ = file_->GetFileType();
                    movie_ = file_->GetMovie();
                    // mdat_ = FindChild("mdat");
                    // if (ftyp_atom_ && movie_ && mdat_)
                    if (ftyp_atom_ && movie_)
                    {
                        // mdat_head_size_ = mdat_->GetHeaderSize();
                        mdat_head_size_ = 8;
                        head_size_ = CaclHeadSize();
                        check_head_is_ok_ = true;
                    }
                    else
                    {
                        Clean();
                        check_head_is_ok_ = false;
                        ec = error::invalid_mp4_head;
                    }
                }
            }
            catch (...)
            {
                ec = error::invalid_mp4_head;
                check_head_is_ok_ = false;
            }

            return ec;
        }

        error_code PptvMp4Head::WriteToMemStream(char const * str, boost::int32_t n)
        {
            stream_->Seek(end_);
            AP4_Result result = stream_->Write(str, n);
            if (AP4_SUCCEEDED(result))
            {
                end_ += n;
            }
            error_code ec;
            if (!AP4_SUCCEEDED(result)) {
                ec = error::invalid_mp4_head;
            }
            return ec;
        }

        error_code PptvMp4Head::WriteHeadToMemStream(error_code & ec)
        {
            if (check_head_is_ok_)
            {
                head_stream_ = new AP4_MemoryByteStream(1024*1024*4);
                if (head_stream_)
                {
                    AP4_Cardinal n = file_->GetTopLevelAtoms().ItemCount();
                    AP4_List<AP4_Atom>::Item *item = file_->GetTopLevelAtoms().FirstItem();
                    while (item != NULL)
                    {
                        AP4_Atom * pAtom = item->GetData();
                        pAtom->Write(*head_stream_);
                        item=item->GetNext();
                    }
                } else {
                    ec = error::invalid_mp4_head;
                }
            } else {
                ec = error::invalid_mp4_head;
            }

            return ec;
        }

        AP4_LargeSize PptvMp4Head::GetHeadStreamSize()
        {
            AP4_Position end = 0;
            head_stream_->Tell(end);
            return end;
        }

        boost::uint64_t PptvMp4Head::GetHeadSize()
        {
            return head_size_;
        }

        boost::uint64_t PptvMp4Head::GetContentLength()
        {
            return content_length_;
        }

        boost::uint64_t PptvMp4Head::GetContentSize()
        {
            return content_size_;
        }

        error_code PptvMp4Head::ReadFromHeadStream(void * p, AP4_Size size)
        {
            error_code ec;
            head_stream_->Seek(0);
            if(!AP4_SUCCEEDED(head_stream_->Read(p,size))) {
                ec = error::invalid_mp4_head;
            }
            return ec;
        }

        error_code PptvMp4Head::SeekHeadStream(AP4_Position position)
        {
            error_code ec;
            if (!AP4_SUCCEEDED(head_stream_->Seek(position))) {
                ec = error::invalid_mp4_head;
            }
            return ec;
        }

        error_code PptvMp4Head::CompareStsdDescription(AP4_Track *cur_track, AP4_Track *source_track)
        {
            error_code ec;
            AP4_StsdAtom *pCurStsdAtom = NULL;
            AP4_StsdAtom *pSourceStsdAtom = NULL;
            char *strStsdPath="mdia/minf/stbl/stsd";
            AP4_Cardinal nCurSize=0;
            AP4_Cardinal nSourceSize=0;
            boost::int32_t i = 0;

            pCurStsdAtom = (AP4_StsdAtom*)FindTrackChild(cur_track, strStsdPath);
            if (pCurStsdAtom == NULL)
            {
                ec = error::invalid_mp4_head;
                return ec;
            }
            pSourceStsdAtom = (AP4_StsdAtom*)FindTrackChild(source_track,strStsdPath);
            if (pSourceStsdAtom == NULL)
            {
                ec = error::invalid_mp4_head;
                return ec;
            }
            nCurSize = pCurStsdAtom->GetSampleDescriptionCount();
            nSourceSize = pSourceStsdAtom->GetSampleDescriptionCount();

            if (nCurSize != nSourceSize)
            {
                ec = error::invalid_mp4_head;
                return ec;
            }

            for (i = 0; i < nCurSize; ++i)
            {
                AP4_SampleDescription *pCurDescription = cur_track->GetSampleDescription(i);
                AP4_SampleDescription *pSourceDescription = source_track->GetSampleDescription(i);
                if (pCurDescription == NULL || pSourceDescription == NULL)
                {
                    continue;
                }
                if (pCurDescription->GetType() != pSourceDescription->GetType())
                {
                    ec = error::invalid_mp4_head;
                    return ec;
                }
                if (pCurDescription->GetFormat() != pSourceDescription->GetFormat())
                {
                    ec = error::invalid_mp4_head;
                    return ec;
                }
            }

            return ec;
        }

        error_code PptvMp4Head::FindTrack(PptvMp4Head & mp4_head,
                                          AP4_Track *cur_track,
                                          AP4_Track **p)
        {
            error_code ec;
            AP4_List<AP4_Track>::Item *item = mp4_head.movie_->GetTracks().FirstItem();
            while (item != NULL)
            {
                AP4_Track *track = item->GetData();
                if (!CompareStsdDescription(cur_track, track))
                {
                    *p = track;
                    return ec;
                }
                item=item->GetNext();
            }
            ec = error::invalid_mp4_truck;
            return ec;
        }

        AP4_Atom* PptvMp4Head::FindTrackChild(AP4_Track * track, char const * path)
        {
            return track->GetTrakAtom()->FindChild(path);
        }

        error_code PptvMp4Head::MergeTrack(PptvMp4Head &mp4_head,
                                   AP4_Track *cur_track,
                                   AP4_Track *source_track,
                                   AP4_StcoAtom **pp_cur_stco)
        {
            error_code   ec;
            AP4_TkhdAtom *pCurTkhd=NULL;
            AP4_TkhdAtom *pSourceTkhd=NULL;
            AP4_SttsAtom *pCurStts=NULL;
            AP4_SttsAtom *pSourceStts=NULL;
            AP4_CttsAtom *pCurCtts=NULL;
            AP4_CttsAtom *pSourceCtts=NULL;
            AP4_StscAtom *pCurStsc=NULL;
            AP4_StscAtom *pSourceStsc=NULL;
            AP4_StszAtom *pCurStsz=NULL;
            AP4_StszAtom *pSourceStsz=NULL;
            AP4_StcoAtom *pCurStco=NULL;
            AP4_StcoAtom *pSourceStco=NULL;
            AP4_StssAtom *pCurStss=NULL;
            AP4_StssAtom *pSourceStss=NULL;
            char *strTkhdPath="tkhd";
            char *strSttsPath="mdia/minf/stbl/stts";
            char *strCttsPath="mdia/minf/stbl/ctts";
            char *strStscPath="mdia/minf/stbl/stsc";
            char *strStszPath="mdia/minf/stbl/stsz";
            char *strStcoPath="mdia/minf/stbl/stco";
            char *strStssPath="mdia/minf/stbl/stss";
            AP4_Cardinal nSize = 0;
            AP4_Cardinal nCurSize = 0;
            AP4_Cardinal nCurChunkCount = 0;
            AP4_Cardinal nChunkCount = 0;
            AP4_Cardinal i = 0;
            AP4_Cardinal nCurSampleCount = cur_track->GetSampleCount();

            //合并tkhd
            pCurTkhd = (AP4_TkhdAtom *)FindTrackChild(cur_track, strTkhdPath);
            if (pCurTkhd == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            pSourceTkhd=(AP4_TkhdAtom *)FindTrackChild(source_track, strTkhdPath);
            if (source_track == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }

            pCurTkhd->SetDuration(pCurTkhd->GetDuration()+pSourceTkhd->GetDuration());
            pCurTkhd->GetParent()->OnChildChanged(pCurTkhd);

            //合并stts
            pCurStts = (AP4_SttsAtom *)FindTrackChild(cur_track, strSttsPath);
            if (pCurStts == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            pSourceStts = (AP4_SttsAtom *)mp4_head.FindTrackChild(source_track, strSttsPath);
            if (pSourceStts == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }

            AP4_Array<AP4_SttsTableEntry> const & source_stts_entries = pSourceStts->GetEntries();
            for (AP4_Ordinal i=0; i < source_stts_entries.ItemCount(); i++) {
                pCurStts->AddEntry(source_stts_entries[i].m_SampleCount, source_stts_entries[i].m_SampleDuration);
            }
            pCurStts->GetParent()->OnChildChanged(pCurStts);

            //合并ctts
            pCurCtts = (AP4_CttsAtom*)FindTrackChild(cur_track, strCttsPath);
            if (pCurCtts != NULL)
            {
                pSourceCtts = (AP4_CttsAtom*)mp4_head.FindTrackChild(source_track, strCttsPath);
                if (pSourceCtts == NULL)
                {
                    ec = error::invalid_mp4_truck;
                    return ec;
                }

                AP4_Array<AP4_CttsTableEntry> const & source_ctts_entries = pSourceCtts->GetEntries();
                for (AP4_Ordinal i=0; i < source_ctts_entries.ItemCount(); i++) {
                    pCurCtts->AddEntry(source_ctts_entries[i].m_SampleCount, source_ctts_entries[i].m_SampleOffset);
                }
                pCurCtts->GetParent()->OnChildChanged(pCurCtts);
            }

            //find stco
            pCurStco = (AP4_StcoAtom *)FindTrackChild(cur_track, strStcoPath);
            if (pCurStco == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            *pp_cur_stco = pCurStco;

            pSourceStco = (AP4_StcoAtom *)mp4_head.FindTrackChild(source_track, strStcoPath);
            if (pSourceStco == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            nCurChunkCount = pCurStco->GetChunkCount();
            nChunkCount = pSourceStco->GetChunkCount();

            //合并stsc
            pCurStsc = (AP4_StscAtom *)FindTrackChild(cur_track, strStscPath);
            if (pCurStsc == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }

            pSourceStsc = (AP4_StscAtom *)mp4_head.FindTrackChild(source_track, strStscPath);
            if (pSourceStsc==NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }

            AP4_StscTableEntry CurStscTableEntry;
            AP4_Ordinal sample_description_index_base = 1;
            AP4_Array<AP4_StscTableEntry> const & cur_stsc_entries = pCurStsc->GetEntries();
            if (cur_stsc_entries.ItemCount() > 0) {
                CurStscTableEntry = cur_stsc_entries[cur_stsc_entries.ItemCount() - 1];
                CurStscTableEntry.m_ChunkCount =
                    nCurChunkCount - cur_stsc_entries[cur_stsc_entries.ItemCount() - 1].m_FirstChunk + 1;
                pCurStsc->SetEntry(cur_stsc_entries.ItemCount() - 1, CurStscTableEntry);
                sample_description_index_base = cur_stsc_entries[cur_stsc_entries.ItemCount() - 1].m_SampleDescriptionIndex;
            }
            
            AP4_Array<AP4_StscTableEntry> const & source_stsc_entries = pSourceStsc->GetEntries();
            for (AP4_Ordinal i=0; i < source_stsc_entries.ItemCount(); ++i) {
                if (source_stsc_entries[i].m_ChunkCount == 0)
                {
                    pCurStsc->AddEntry(nChunkCount - source_stsc_entries[i].m_FirstChunk + 1,
                                       source_stsc_entries[i].m_SamplesPerChunk,
                                       sample_description_index_base);
                }
                else
                {
                    pCurStsc->AddEntry(source_stsc_entries[i].m_ChunkCount,
                                       source_stsc_entries[i].m_SamplesPerChunk,
                                       sample_description_index_base);
                }
            }
            pCurStsc->GetParent()->OnChildChanged(pCurStsc);

            //合并stsz
            pCurStsz = (AP4_StszAtom*)FindTrackChild(cur_track, strStszPath);
            if (pCurStsz != NULL)
            {
                pSourceStsz = (AP4_StszAtom*)FindTrackChild(source_track, strStszPath);
                if (pSourceStsz == NULL)
                {
                    ec = error::invalid_mp4_truck;
                    return ec;
                }
                nSize = pSourceStsz->GetSampleCount();
                for (i = 1; i <= nSize; i++)
                {
                    AP4_Size sample_size;
                    pSourceStsz->GetSampleSize(i, sample_size);
                    pCurStsz->AddEntry(sample_size);
                }
                pCurStsz->GetParent()->OnChildChanged(pCurStsz);
            }

            //合并stco
            pCurStco->AdjustChunkOffsets(head_size_ * (-1));
            pCurStco->ReSetEntryCount(nCurChunkCount + nChunkCount);
            memcpy(pCurStco->GetChunkOffsets() + nCurChunkCount, pSourceStco->GetChunkOffsets(), nChunkCount*4);
            pCurStco->AdjustChunkOffsets(nCurChunkCount, 
                                         (mp4_head.head_size_) * (-1) + content_size_);
            pCurStco->GetParent()->OnChildChanged(pCurStco);

            //合并stss
            pCurStss=(AP4_StssAtom *)FindTrackChild(cur_track, strStssPath);
            if (pCurStss != NULL)
            {
                pSourceStss = (AP4_StssAtom *)mp4_head.FindTrackChild(source_track, strStssPath);
                if (pSourceStss == NULL)
                {
                    ec = error::invalid_mp4_truck;
                    return ec;
                }
                nCurSize = nCurSampleCount;
                const AP4_Array<AP4_UI32> & SourceStssArray = pSourceStss->GetEntries();
                nSize = SourceStssArray.ItemCount();
                for (i = 0; i < nSize; ++i)
                {
                    pCurStss->AddEntry(SourceStssArray[i] + nCurSize);
                }
                pCurStss->GetParent()->OnChildChanged(pCurStss);
            }

            return ec;
        }

        error_code PptvMp4Head::Merge(PptvMp4Head & mp4_head)
        {
            error_code ec;
            std::vector<AP4_StcoAtom*> StcoAtomVec;
            AP4_StcoAtom *pCurStco = NULL;
            int nSize = 0;

            //init
            content_size_ = content_length_ - head_size_;

            //merge mvhd
            AP4_MvhdAtom *pCurMvhd = NULL;
            AP4_MvhdAtom *pSourceMvhd = NULL;
            char *strMvhdPath = "moov/mvhd";

            pCurMvhd =(AP4_MvhdAtom*)FindChild(strMvhdPath);
            if (pCurMvhd == NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            pSourceMvhd = (AP4_MvhdAtom*)mp4_head.FindChild(strMvhdPath);
            if (pSourceMvhd==NULL)
            {
                ec = error::invalid_mp4_truck;
                return ec;
            }
            pCurMvhd->SetDuration(pCurMvhd->GetDuration() + pSourceMvhd->GetDuration());
            pCurMvhd->GetParent()->OnChildChanged(pCurMvhd);

            //merge track
            AP4_List<AP4_Track>::Item *item = movie_->GetTracks().FirstItem();
            while (item != NULL)
            {
                AP4_Track *pCurTrack = item->GetData();
                AP4_Track *pSourceTrack = NULL;

                ec = FindTrack(mp4_head, pCurTrack, &pSourceTrack);
                if (ec) {
                    return ec;
                }

                pCurStco = NULL;
                ec = MergeTrack(mp4_head, pCurTrack, pSourceTrack, &pCurStco);
                if (ec) {
                    return ec;
                }
                StcoAtomVec.push_back(pCurStco);
                item=item->GetNext();
            }

            //end
            head_size_ = CaclHeadSize();
            nSize = StcoAtomVec.size();
            for (int i = 0; i < nSize; ++i)
            {
                pCurStco = StcoAtomVec[i];
                pCurStco->AdjustChunkOffsets(head_size_);
            }

            content_length_ = content_size_ + mp4_head.content_length_ - mp4_head.head_size_ + head_size_;

            return ec;
        }

        AP4_Size PptvMp4Head::CaclHeadSize()
        {
            AP4_Size nSize=0;
            AP4_Cardinal n = file_->GetTopLevelAtoms().ItemCount();
            AP4_List<AP4_Atom>::Item *item = file_->GetTopLevelAtoms().FirstItem();
            while (item != NULL)
            {
                AP4_Atom *pAtom=item->GetData();
                //if (pAtom->GetType()==AP4_ATOM_TYPE_MDAT)
                //{
                //    nSize += mdat_head_size_;
                //    item = item->GetNext();
                //    continue;
                //}
                nSize += pAtom->GetSize();
                item = item->GetNext();
            }
            return nSize + mdat_head_size_;
        }

        void PptvMp4Head::SetContentLength(AP4_Size contentlength)
        {
            content_length_ = contentlength;
        }

        void PptvMp4Head::SetMemStreamEndPosition(AP4_Position pos)
        {
            end_ = pos;
        }

        AP4_Atom* PptvMp4Head::FindChild(const char *path)
        {
            return file_->FindChild(path);
        }

        void PptvMp4Head::Clean()
        {
            if (file_)
            {
                delete file_;
                file_ = NULL;
                mdat_ = NULL;
                movie_ = NULL;
                ftyp_atom_ = NULL;
            }
        }

    } /* namespace download */
} /* namespace ppbox */
