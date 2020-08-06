#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>

#include "sample_comm.h"
#include "audio_aac_adp.h"
#include "audio_dl_adp.h"

#define SA_AE_PT            PT_AAC
#define SA_AE_MODE          AUDIO_SOUND_MODE_STEREO

#define SA_AI_DEV           SAMPLE_AUDIO_INNER_AI_DEV
#define SA_AO_DEV           SAMPLE_AUDIO_INNER_AO_DEV
#define SA_AI_CHN           0
#define SA_AE_CHN           0
#define SA_AD_CHN           0

#define SA_AE_CHNS          1

#define SA_ENABLE_RESAMPLE  HI_FALSE
#define SA_ENABLE_VQE       HI_FALSE

/******************************************************************************
* function : main
******************************************************************************/
HI_VOID SAMPLE_AENC_DeInit()
{
    HI_S32 s32Ret;

    s32Ret = SAMPLE_COMM_AUDIO_AencUnbindAi(SA_AI_DEV , SA_AI_CHN, SA_AE_CHN);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_AencUnbindAi failed with %#x!\n", s32Ret);
    }

    s32Ret = SAMPLE_COMM_AUDIO_StopAenc(SA_AE_CHNS);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAenc failed with %#x!\n", s32Ret);
    }

    s32Ret = SAMPLE_COMM_AUDIO_StopAi(SA_AI_DEV, 1 << SA_AE_MODE, SA_ENABLE_RESAMPLE, SA_ENABLE_VQE);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAi failed with %#x!\n", s32Ret);
    }

    s32Ret = HI_MPI_AENC_AacDeInit();
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_AENC_AacDeInit failed with %#x!\n", s32Ret);
    }
}

HI_S32 SAMPLE_AENC_Init(AUDIO_SAMPLE_RATE_E sampleRate, AENC_CHN numChannels)
{
    HI_S32      s32Ret;
    AIO_ATTR_S  stAioAttr;

    s32Ret = HI_MPI_AENC_AacInit();
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_AENC_AacInit failed with %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    stAioAttr.enSamplerate   = sampleRate;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SA_AE_MODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = AACLC_SAMPLES_PER_FRAME;
    stAioAttr.u32ChnCnt      = numChannels;
#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.u32ClkSel      = 1;
    stAioAttr.enI2sType      = AIO_I2STYPE_EXTERN;
#else
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;
#endif

    /********************************************
      step 1: start Ai
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(SA_AI_DEV, stAioAttr.u32ChnCnt, &stAioAttr, \
        AUDIO_SAMPLE_RATE_BUTT, SA_ENABLE_RESAMPLE, NULL, 0);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAi failed with %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    /********************************************
      step 2: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_CfgAcodec failed with %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(SA_AE_CHNS, &stAioAttr, SA_AE_PT);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAenc failed with %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(SA_AI_DEV, SA_AI_CHN, SA_AE_CHN);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_AencBindAi failed with %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }
    SAMPLE_PRT("Ai(%d,%d) bind to AencChn:%d ok!\n", SA_AI_DEV , SA_AI_CHN, SA_AE_CHN);
  
    return HI_SUCCESS;

EXIT_ERROR:
    SAMPLE_AENC_DeInit();

    return s32Ret;
}

HI_S32 SAMPLE_AENC_PeekStream(AUDIO_STREAM_S *pstStream) {
    HI_S32              s32Ret;
    HI_S32              aecFd;
    fd_set              readFds;
    struct timeval      timeVal = {1, 0};
    AI_CHN_PARAM_S      stAiChnPara;

    aecFd = HI_MPI_AENC_GetFd(SA_AE_CHN);
    if (aecFd < 0) {
        SAMPLE_PRT("HI_MPI_AENC_GetFd failed with error %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    //SAMPLE_PRT("ai fd of chn %d is %d!\n", SA_AI_CHN, aiFd);

    FD_ZERO(&readFds);
    FD_SET(aecFd, &readFds);

    int ret = select(aecFd + 1, &readFds, NULL, NULL, &timeVal);

    if (ret < 0) {
        printf("select failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    if (ret == 0) {
        SAMPLE_PRT("select time out\n");
        return HI_FAILURE;
    }

    if (! FD_ISSET(aecFd, &readFds)) {
        return HI_FAILURE;
    }

    memset(&pstStream, 0, sizeof(AUDIO_STREAM_S));
    s32Ret = HI_MPI_AENC_GetStream(SA_AE_CHN, pstStream, HI_FALSE);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_AI_GetFrame failed with error %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AENC_ReleaseStream(AUDIO_STREAM_S *pstStream) {
    HI_S32 s32Ret;

    s32Ret = HI_MPI_AENC_ReleaseStream(SA_AE_CHN, pstStream);
    if (HI_SUCCESS != s32Ret ) {
        SAMPLE_PRT("HI_MPI_AENC_ReleaseStream failed with error %#x!\n", s32Ret);
    }

    return s32Ret;
}
