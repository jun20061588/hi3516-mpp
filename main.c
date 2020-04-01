#include <stdio.h>
#include <pthread.h>

#include "sample_comm.h"

/******************************************************************************
* function: H.265e + H264e@720P, H.265 Channel resolution adaptable with sensor
******************************************************************************/

#define SV_CHANNEL_NUM  3

int main(int argc, char ** argv) {
    HI_S32          s32Ret;
    VENC_CHN        VencChn[VPSS_MAX_PHY_CHN_NUM];
    PIC_SIZE_E      enSize[SV_CHANNEL_NUM] = {PIC_1080P, PIC_720P, PIC_360P};
    VENC_STREAM_S   stStream;
    HI_U32          u32rameSize = 0;

    for (HI_S32 chn = 0; chn < SV_CHANNEL_NUM; ++chn) {
        VencChn[chn] = chn;
    }

    if (HI_SUCCESS != SAMPLE_VENC_Init(enSize, SV_CHANNEL_NUM)) {
        return -1;
    }

    /******************************************
     stream save process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, SV_CHANNEL_NUM);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("Start Venc failed!\n");
        SAMPLE_VENC_DeInit();
    }

    while(1) {
        if (HI_SUCCESS == SAMPLE_VENC_PeekStream(0, &stStream)) {
            /*******************************************************
             step 2.5 : save frame
            *******************************************************/
            u32rameSize = 0;
            for (int i = 0; i < stStream.u32PackCount; ++i) {
                printf("u32Offset = %d, u32Len = %d\n",
                    stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len);
                u32rameSize += stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
            }
            printf("u32rameSize = %d\n", u32rameSize);
            SAMPLE_VENC_ReleaseStream(0, &stStream);
        }
        // usleep(10000);
    }

    return 0;
}