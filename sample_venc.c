/******************************************************************************

  Copyright (C), 2017, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_venc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2017
  Description   :
******************************************************************************/

#include "sample_comm.h"

#include <sys/select.h>
#include <sys/types.h>

#define SV_EN_DYNAMIC_RANGE    DYNAMIC_RANGE_SDR8
#define SV_EN_PIXEL_FORMAT     PIXEL_FORMAT_YVU_SEMIPLANAR_420

#define SV_VI_DEV0             0
#define SV_VI_PIPE0            0
#define SV_VI_CHN0             0
#define SV_VPSS_GRP            0
#define SV_PAYLOAD_TYPE        PT_H264
#define SV_GOP_MODE            VENC_GOPMODE_NORMALP
#define SV_RC_MODE             SAMPLE_RC_CBR

static SAMPLE_VI_CONFIG_S gstViConfig;
static HI_BOOL gbChnEnabled[VPSS_MAX_PHY_CHN_NUM];

HI_S32 SAMPLE_VENC_SYS_Init(SIZE_S stSize[], HI_U16 uChnNum) {
    HI_S32        s32Ret;
    HI_U64        u64BlkSize;
    VB_CONFIG_S   stVbConf;
    
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    stVbConf.u32MaxPoolCnt = uChnNum;
    for (HI_U32 chn = 0; chn < uChnNum; ++chn) {
        u64BlkSize = COMMON_GetPicBufferSize( \
            stSize[chn].u32Width, stSize[chn].u32Height, \
            SV_EN_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG,DEFAULT_ALIGN);
        stVbConf.astCommPool[chn].u64BlkSize  = u64BlkSize;
        stVbConf.astCommPool[chn].u32BlkCnt   = 10;
    }

    return SAMPLE_COMM_SYS_Init(&stVbConf);
}

HI_S32 SAMPLE_VENC_VI_Init() {
    gstViConfig.s32WorkingViNum                          = 1;
    gstViConfig.as32WorkingViId[0]                       = 0;

    gstViConfig.astViInfo[0].stSnsInfo.MipiDev           = 
        SAMPLE_COMM_VI_GetComboDevBySensor(gstViConfig.astViInfo[0].stSnsInfo.enSnsType, 0);
    gstViConfig.astViInfo[0].stSnsInfo.s32BusId          = 0;

    gstViConfig.astViInfo[0].stDevInfo.ViDev             = SV_VI_DEV0;
    gstViConfig.astViInfo[0].stDevInfo.enWDRMode         = WDR_MODE_NONE;
    gstViConfig.astViInfo[0].stPipeInfo.enMastPipeMode   = VI_OFFLINE_VPSS_OFFLINE;
    
    gstViConfig.astViInfo[0].stPipeInfo.aPipe[0]         = SV_VI_PIPE0;
    gstViConfig.astViInfo[0].stPipeInfo.aPipe[1]         = -1;
    gstViConfig.astViInfo[0].stPipeInfo.aPipe[2]         = -1;
    gstViConfig.astViInfo[0].stPipeInfo.aPipe[3]         = -1;

    gstViConfig.astViInfo[0].stChnInfo.ViChn             = SV_VI_CHN0;
    gstViConfig.astViInfo[0].stChnInfo.enPixFormat       = SV_EN_PIXEL_FORMAT;
    gstViConfig.astViInfo[0].stChnInfo.enDynamicRange    = SV_EN_DYNAMIC_RANGE;
    gstViConfig.astViInfo[0].stChnInfo.enVideoFormat     = VIDEO_FORMAT_LINEAR;
    gstViConfig.astViInfo[0].stChnInfo.enCompressMode    = COMPRESS_MODE_SEG;//COMPRESS_MODE_SEG;

    return SAMPLE_COMM_VI_StartVi(&gstViConfig);
}
    
HI_S32 SAMPLE_VENC_VPSS_Init(SIZE_S stSnsSize, SIZE_S stSize[]) {
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    stVpssGrpAttr.enPixelFormat                 = SV_EN_PIXEL_FORMAT;
    stVpssGrpAttr.enDynamicRange                = SV_EN_DYNAMIC_RANGE;
    stVpssGrpAttr.u32MaxW                       = stSnsSize.u32Width;
    stVpssGrpAttr.u32MaxH                       = stSnsSize.u32Height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate   = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate   = -1;
    stVpssGrpAttr.bNrEn                         = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enNrType             = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enNrMotionMode       = NR_MOTION_MODE_NORMAL;
    stVpssGrpAttr.stNrAttr.enCompressMode       = COMPRESS_MODE_FRAME;

    for(HI_S32 chn = 0; chn < VPSS_MAX_PHY_CHN_NUM; ++chn) {
        if(HI_TRUE == gbChnEnabled[chn]) {
            stVpssChnAttr[chn].u32Width                     = stSize[chn].u32Width;
            stVpssChnAttr[chn].u32Height                    = stSize[chn].u32Height;
            stVpssChnAttr[chn].enChnMode                    = VPSS_CHN_MODE_USER;
            stVpssChnAttr[chn].enCompressMode               = COMPRESS_MODE_NONE;//COMPRESS_MODE_SEG;
            stVpssChnAttr[chn].enDynamicRange               = SV_EN_DYNAMIC_RANGE;
            stVpssChnAttr[chn].enPixelFormat                = SV_EN_PIXEL_FORMAT;
            stVpssChnAttr[chn].stFrameRate.s32SrcFrameRate  = -1;
            stVpssChnAttr[chn].stFrameRate.s32DstFrameRate  = -1;
            stVpssChnAttr[chn].u32Depth                     = 0;
            stVpssChnAttr[chn].bMirror                      = HI_FALSE;
            stVpssChnAttr[chn].bFlip                        = HI_FALSE;
            stVpssChnAttr[chn].enVideoFormat                = VIDEO_FORMAT_LINEAR;
            stVpssChnAttr[chn].stAspectRatio.enMode         = ASPECT_RATIO_NONE;
        }
    }

    return SAMPLE_COMM_VPSS_Start(SV_VPSS_GRP, gbChnEnabled, &stVpssGrpAttr, stVpssChnAttr);
}

HI_VOID SAMPLE_VENC_DeInit() {
    SAMPLE_PRT("TRACE\n");
    SAMPLE_COMM_VENC_StopGetStream();
    for (HI_S32 chn = 0; chn < VPSS_MAX_PHY_CHN_NUM; ++chn) {
        if (HI_TRUE == gbChnEnabled[chn]) {
            SAMPLE_COMM_VPSS_UnBind_VENC(SV_VPSS_GRP, chn, chn);
            SAMPLE_COMM_VENC_Stop(chn);
        }
    }
    SAMPLE_COMM_VI_UnBind_VPSS(SV_VI_PIPE0, SV_VI_CHN0, SV_VPSS_GRP);
    SAMPLE_COMM_VPSS_Stop(SV_VPSS_GRP, gbChnEnabled);
    SAMPLE_COMM_VI_StopVi(&gstViConfig);
    SAMPLE_COMM_SYS_Exit();
}

HI_S32 SAMPLE_VENC_Init(PIC_SIZE_E enSize[], HI_U32 uChnNum) {
    HI_S32          s32Ret;
    SIZE_S          stSnsSize;
    PIC_SIZE_E      enSnsSize;
    VENC_GOP_ATTR_S stGopAttr;

    SIZE_S          stSize[VPSS_MAX_PHY_CHN_NUM];
    VENC_CHN        VencChn[VPSS_MAX_PHY_CHN_NUM];
    VPSS_CHN        VpssChn[VPSS_MAX_PHY_CHN_NUM];

    SAMPLE_COMM_VI_GetSensorInfo(&gstViConfig);
    if (SAMPLE_SNS_TYPE_BUTT == gstViConfig.astViInfo[0].stSnsInfo.enSnsType) {
        SAMPLE_PRT("Not set SENSOR%d_TYPE !\n",0);
        goto EXIT_ERROR;
    }

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(
        gstViConfig.astViInfo[0].stSnsInfo.enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    /******************************************
     init arrays
    *******************************************/
    memset(gbChnEnabled, 0, sizeof(gbChnEnabled));
    for (HI_S32 chn = 0; chn < uChnNum; ++chn) {
        gbChnEnabled[chn] = HI_TRUE;
        SAMPLE_COMM_SYS_GetPicSize(enSize[chn], &stSize[chn]);
    }

    /******************************************
     check sensor
    *******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }
    SAMPLE_PRT("stSnsSize = (%d %d)!\n", stSnsSize.u32Width, stSnsSize.u32Height);

    for (HI_S32 chn = 0; chn < uChnNum; ++chn) {
        SAMPLE_PRT("stSize[%d] = (%d %d)!\n", chn, stSize[chn].u32Width, stSize[chn].u32Height);
        if ((stSnsSize.u32Width < stSize[chn].u32Width) \
            || (stSnsSize.u32Height < stSize[chn].u32Height)) {
            SAMPLE_PRT("Sensor size is (%d,%d), but encode size is (%d,%d) !\n",
                stSnsSize.u32Width, stSnsSize.u32Height,
                stSize[chn].u32Width, stSize[chn].u32Height);
            goto EXIT_ERROR;
        }
    }

    s32Ret = SAMPLE_VENC_SYS_Init(stSize, uChnNum);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_VENC_SYS_Init failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    s32Ret = SAMPLE_VENC_VI_Init();
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_VENC_VI_Init failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    s32Ret = SAMPLE_VENC_VPSS_Init(stSnsSize, stSize);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_VENC_VPSS_Init failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(SV_VI_PIPE0, SV_VI_CHN0, SV_VPSS_GRP);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VI_Bind_VPSS failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }

    /******************************************
     start stream venc
    *******************************************/
    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(SV_GOP_MODE, &stGopAttr);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VENC_GetGopAttr failed with error %#x!\n", s32Ret);
        goto EXIT_ERROR;
    }
    for (HI_S32 chn = 0; chn < uChnNum; ++chn) {
        s32Ret = SAMPLE_COMM_VENC_Start(chn, SV_PAYLOAD_TYPE, enSize[0], \
            SV_RC_MODE, 0,HI_TRUE,&stGopAttr);
        if (HI_SUCCESS != s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VENC_Start failed with error %#x!\n", s32Ret);
            goto EXIT_ERROR;
        }

        s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(SV_VPSS_GRP, chn, chn);
        if (HI_SUCCESS != s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VENC failed with error %#x!\n", s32Ret);
            goto EXIT_ERROR;
        }
    }

    return HI_SUCCESS;

EXIT_ERROR:
    SAMPLE_VENC_DeInit();

    return s32Ret;
}

HI_S32 SAMPLE_VENC_PeekStream(VENC_CHN vencChn, VENC_STREAM_S *pstStream) {
    HI_S32              s32Ret;
    VENC_CHN_STATUS_S   stStat;
    fd_set              readFds = {0};
    struct timeval      timeVal = {2, 0};

    int vencFd = HI_MPI_VENC_GetFd(vencChn);
    if (vencFd < 0) {
        SAMPLE_PRT("HI_MPI_VENC_GetFd failed with error %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    //SAMPLE_PRT("Venc fd of chn %d is %d!\n", vencChn, vencFd);

    FD_ZERO(&readFds);
    FD_SET(vencFd, &readFds);

    int ret = select(vencFd + 1, &readFds, NULL, NULL, &timeVal);

    if (ret < 0) {
        printf("select failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    if (ret == 0) {
        SAMPLE_PRT("select time out\n");
        return HI_FAILURE;
    }

    if (! FD_ISSET(vencFd, &readFds)) {
        return HI_FAILURE;
    }

    memset(pstStream, 0, sizeof(VENC_STREAM_S));
    /*******************************************************
     step 2.1 : query how many packs in one-frame stream.
    *******************************************************/
    s32Ret = HI_MPI_VENC_QueryStatus(vencChn, &stStat);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", vencChn, s32Ret);
        return HI_FAILURE;
    }
    /*******************************************************
    step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
    *******************************************************/
    if (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames) {
        SAMPLE_PRT("NOTE: Current frame is NULL!\n");
        return HI_FAILURE;
    }
    /*******************************************************
     step 2.3 : malloc corresponding number of pack nodes.
    *******************************************************/
    pstStream->pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
    if (NULL == pstStream->pstPack) {
        SAMPLE_PRT("malloc stream pack failed!\n");
        return HI_FAILURE;
    }
    //SAMPLE_PRT("TRACE: pstStream->pstPack = %p!\n", pstStream->pstPack);
    /*******************************************************
     step 2.4 : call mpi to get one-frame stream
    *******************************************************/
    pstStream->u32PackCount = stStat.u32CurPacks;
    s32Ret = HI_MPI_VENC_GetStream(vencChn, pstStream, HI_TRUE);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
        free(pstStream->pstPack);
        pstStream->pstPack = NULL;
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VENC_ReleaseStream(VENC_CHN vencChn, VENC_STREAM_S *pstStream) {
    HI_S32  s32Ret;

    s32Ret = HI_MPI_VENC_ReleaseStream(vencChn, pstStream);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
    }

    //SAMPLE_PRT("TRACE: pstStream->pstPack = %p!\n", pstStream->pstPack);
    if (pstStream->pstPack) {
        free(pstStream->pstPack);
        pstStream->pstPack = NULL;
    }
    return s32Ret;
}
