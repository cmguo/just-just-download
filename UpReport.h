#ifndef _PPBOX_UP_REPORT_H_
#define _PPBOX_UP_REPORT_H_

namespace ppbox
{
    namespace download
    {
        struct ReportData
        {
            ReportData(PP_uint64 a,boost::uint32_t b)
                :curr_data_size(a)
                ,curr_time(b)
            {

            }
            boost::uint64_t curr_data_size;  //˲ʱ���ش�С
            boost::uint32_t curr_time; //��ǰ������ʱ��
        };

        class UpReport
        {
        public:
            virtual void up_report(ReportData const & rdata)
            {
                std::cout<<"up_report Wrong"<<std::endl;
            }
        };

    } // namespace download
} // namespace ppbox

#endif 