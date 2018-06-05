/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : EncoderTest.c
 * Description : EncoderTest
 * History :
 *
 */

#include <cdx_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "vencoder.h"
#include <sys/time.h>
#include <time.h>
#include <memoryAdapter.h>

#define DEMO_FILE_NAME_LEN 256
//#define USE_VIDEO_SIGNAL
//#define USE_ASPECT_RATIO
//#define USE_SUPER_FRAME

#define _ENCODER_TIME_
#ifdef _ENCODER_TIME_
//extern int gettimeofday(struct timeval *tv, struct timezone *tz);
static long long GetNowUs()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

long long time1=0;
long long time2=0;
long long time3=0;
#endif

typedef struct {
    char             intput_file[256];
    char             output_file[256];
    char             reference_file[256];
    int              compare_flag;
    int              compare_result;

    unsigned int  encode_frame_num;
    unsigned int  encode_format;

    unsigned int src_size;
    unsigned int dst_size;
    unsigned int output_len;

    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_width;
    unsigned int dst_height;

    bool bSvc;
    int qpmin;
    int qpmax;
    int bit_rate;
    int frame_rate;
    int maxKeyFrame;
}encode_param_t;

typedef enum {
    INPUT,
    HELP,
    ENCODE_FRAME_NUM,
    ENCODE_FORMAT,
    OUTPUT,
    SRC_SIZE,
    DST_SIZE,
    COMPARE_FILE,
    BITRATE,
    FRAMERATE,
    INVALID
}ARGUMENT_T;

typedef struct {
    char Short[8];
    char Name[128];
    int argument;
    char Description[512];
}argument_t;

static const argument_t ArgumentMapping[] =
{
    { "-h",  "--help",    1,
        "Print this help" },
    { "-i",  "--input",   2,
        "Input file path" },
    { "-n",  "--encode_frame_num",   3,
        "After encoder n frames, encoder stop" },
    { "-c",  "--encode_format",  4,
        "0:h264 encoder, 1:jpeg_encoder" },
    { "-o",  "--output", 5,
        "output file path" },
    { "-s",  "--srcsize",  6,
        "src_size,can be 1080,720,480" },
    { "-d",  "--dstsize",  7 ,
        "dst_size,can be 1080,720,480" },
    { "--",  "--compare",  8,
        "compare file:reference file path" },
    { "--",  "-qpmin",  9,
        "set min qp: " },
    { "--",  "-qpmax",  10,
        "set max qp: " },
    { "-b",  "--bitrate",  11,
        "bitrate setting for example 1000000(1M)" },
    { "-f",  "--framerate",  12,
        "framerate setting for example 30/60" },
    { "--",  "-avc",  12,
        "set as avc encoding " },
};

int yu12_nv12(unsigned int width, unsigned int height, unsigned char *addr_uv,
          unsigned char *addr_tmp_uv)
{
    unsigned int i, chroma_bytes;
    unsigned char *u_addr = NULL;
    unsigned char *v_addr = NULL;
    unsigned char *tmp_addr = NULL;

    chroma_bytes = width*height/4;

    u_addr = addr_uv;
    v_addr = addr_uv + chroma_bytes;
    tmp_addr = addr_tmp_uv;

    for(i=0; i<chroma_bytes; i++)
    {
        *(tmp_addr++) = *(u_addr++);
        *(tmp_addr++) = *(v_addr++);
    }

    memcpy(addr_uv, addr_tmp_uv, chroma_bytes*2);

    return 0;
}



static void PrintDemoUsage(void)
{
    int i = 0;
    int num = sizeof(ArgumentMapping) / sizeof(argument_t);
    printf("Usage:");
    while(i < num)
    {
        printf("%s %-32s %s", ArgumentMapping[i].Short, ArgumentMapping[i].Name,
                ArgumentMapping[i].Description);
        printf("\n");
        i++;
    }
}

void ParseArgument(encode_param_t *encode_param, char argc, char **argv)
{
    const struct option long_opts[] = {
        {"help", no_argument, NULL, 0 },
        {"bitrate", required_argument, NULL, 1 },
        {"compare", required_argument, NULL, 2 },
        {"qpmin", required_argument, NULL, 3 },
        {"qpmax", required_argument, NULL, 4 },
        {"avc", no_argument, NULL, 5 },
        {"hh", no_argument, NULL, 69},
        {NULL, no_argument, NULL, 0 }
    };

    int long_index, i ;
    int c;
    while (1){
	c = getopt_long_only(argc, argv, "b:s:i:d:c:n:f:o:", long_opts, &long_index);

	if (c == -1)
		break;

        switch (c) {
        case 69:
        case 0:
            PrintDemoUsage();
            //PrintHelpInfo();
            exit(0);
        case  'b':
            encode_param->bit_rate= atoi(optarg);
            break;
        case  3:
            encode_param->qpmin= atoi(optarg);
            break;
        case  4:
            encode_param->qpmax= atoi(optarg);
            break;
        case  5:
            encode_param->bSvc = false;
            break;
        case  2:
            memset(encode_param->reference_file, 0, sizeof(encode_param->reference_file));
            strncpy(encode_param->reference_file, optarg, strlen(optarg));
            encode_param->compare_flag = 1;
            break;
        case 's':
            encode_param->src_size= atoi(optarg);
	    if(encode_param->src_size == 1080)
	    {
		    encode_param->src_width = 1920;
		    encode_param->src_height = 1080;
	    }
	    else if(encode_param->src_size == 720)
	    {
		    encode_param->src_width = 1280;
		    encode_param->src_height = 720;
            }
            else if(encode_param->src_size == 480)
            {
                encode_param->src_width = 640;
                encode_param->src_height = 480;
            }
            else
            {
                encode_param->src_width = 1280;
                encode_param->src_height = 720;
                printf("encoder demo only support the size 1080p,720p,480p, \
                 now use the default size 720p\n");
            }

            break;
        case 'd':
            encode_param->dst_size= atoi(optarg);
            if(encode_param->dst_size == 1080)
            {
                encode_param->dst_width = 1920;
                encode_param->dst_height = 1080;
            }
            else if(encode_param->dst_size == 720)
            {
                encode_param->dst_width = 1280;
                encode_param->dst_height = 720;
            }
            else if(encode_param->dst_size == 480)
            {
                encode_param->dst_width = 640;
                encode_param->dst_height = 480;
            }
            else
            {
                encode_param->dst_width = 1280;
                encode_param->dst_height = 720;
                printf("encoder demo only support the size 1080p,720p,480p,\
                 now use the default size 720p\n");
            }

            break;
        case 'c':
            encode_param->encode_format = atoi(optarg);
            break;
        case 'n':
            encode_param->encode_frame_num= atoi(optarg);
            break;
        case 'f':
            encode_param->frame_rate = atoi(optarg);
            break;
        case 'i':
            memset(encode_param->intput_file, 0 , sizeof(encode_param->intput_file));
            strncpy(encode_param->intput_file, optarg, strlen(optarg));
            break;
        case 'o':
            memset(encode_param->output_file, 0 , sizeof(encode_param->output_file));
            strncpy(encode_param->output_file, optarg, strlen(optarg));
            break;

	default:
	    break;
	}
    }
}

void DemoHelpInfo(void)
{
    logd(" ==== CedarX2.0 encoder demo help start ===== ");
    logd(" -h or --help to show the demo usage");
    logd(" demo created by yangcaoyuan, allwinnertech/AL3 ");
    logd(" email: yangcaoyuan@allwinnertech.com ");
    logd(" ===== CedarX2.0 encoder demo help end ====== ");
}

int SeekPrefixNAL(char* begin)
{
    unsigned int i;
    char* pchar = begin;
    char TemporalID = 0;
    char isPrefixNAL = 1;
    char NAL[4] = {0x00, 0x00, 0x00, 0x01};
    char PrefixNAL[3] = {0x6e, 0x4e, 0x0e};

    if(!pchar)
    {
        return -1;
    }

    for(i=0; i<4; i++)
    {
        if(pchar[i] != NAL[i])
        {
            isPrefixNAL = 0;
            break;
        }
    }
    if(isPrefixNAL == 1)
    {
        isPrefixNAL = 0;
        for(i=0; i<3; i++)
        {
            if(pchar[4] == PrefixNAL[i])
            {
                isPrefixNAL = 1;
                break;
            }
        }
    }
    // read temporal_id
    if(isPrefixNAL == 1)
    {
        TemporalID = pchar[7];
        TemporalID >>= 5;
        return TemporalID;
    }

    return -1;
}

void InitJpegExif(EXIFInfo *exifinfo)
{
    exifinfo->ThumbWidth = 640;
    exifinfo->ThumbHeight = 480;

    strcpy((char*)exifinfo->CameraMake,        "allwinner make test");
    strcpy((char*)exifinfo->CameraModel,        "allwinner model test");
    strcpy((char*)exifinfo->DateTime,         "2014:02:21 10:54:05");
    strcpy((char*)exifinfo->gpsProcessingMethod,  "allwinner gps");

    exifinfo->Orientation = 0;

    exifinfo->ExposureTime.num = 2;
    exifinfo->ExposureTime.den = 1000;

    exifinfo->FNumber.num = 20;
    exifinfo->FNumber.den = 10;
    exifinfo->ISOSpeed = 50;

    exifinfo->ExposureBiasValue.num= -4;
    exifinfo->ExposureBiasValue.den= 1;

    exifinfo->MeteringMode = 1;
    exifinfo->FlashUsed = 0;

    exifinfo->FocalLength.num = 1400;
    exifinfo->FocalLength.den = 100;

    exifinfo->DigitalZoomRatio.num = 4;
    exifinfo->DigitalZoomRatio.den = 1;

    exifinfo->WhiteBalance = 1;
    exifinfo->ExposureMode = 1;

    exifinfo->enableGpsInfo = 1;

    exifinfo->gps_latitude = 23.2368;
    exifinfo->gps_longitude = 24.3244;
    exifinfo->gps_altitude = 1234.5;

    exifinfo->gps_timestamp = (long)time(NULL);

    strcpy((char*)exifinfo->CameraSerialNum,  "123456789");
    strcpy((char*)exifinfo->ImageName,  "exif-name-test");
    strcpy((char*)exifinfo->ImageDescription,  "exif-descriptor-test");
}

int main(int argc, char** argv)
{
    VencBaseConfig baseConfig;
    VencAllocateBufferParam bufferParam;
    VideoEncoder* pVideoEnc = NULL;
    VencInputBuffer inputBuffer;
    VencOutputBuffer outputBuffer;
    VencHeaderData sps_pps_data;
    VencH264Param h264Param;
    VencH264FixQP fixQP;
    EXIFInfo exifinfo;
    VencCyclicIntraRefresh sIntraRefresh;
    unsigned char *uv_tmp_buffer = NULL;
#ifdef USE_SUPER_FRAME
    VencSuperFrameConfig     sSuperFrameCfg;
#endif
    VencH264SVCSkip         SVCSkip; // set SVC and skip_frame
#ifdef USE_ASPECT_RATIO
    VencH264AspectRatio        sAspectRatio;
#endif
#ifdef USE_VIDEO_SIGNAL
    VencH264VideoSignal        sVideoSignal;
#endif

    int result = 0;
    int i = 0;
    //long long pts = 0;

    FILE *in_file = NULL;
    FILE *out_file = NULL;
    FILE *reference_file = NULL;

    char *input_path = NULL;
    char *output_path = NULL;
    char *reference_path = NULL;
    unsigned char *reference_buffer = NULL;

    //set the default encode param
    encode_param_t    encode_param;
    memset(&encode_param, 0, sizeof(encode_param));

    encode_param.src_width = 1280;
    encode_param.src_height = 720;
    encode_param.dst_width = 1280;
    encode_param.dst_height = 720;

    encode_param.bit_rate = 2*1024*1024;
    encode_param.frame_rate = 30;
    encode_param.maxKeyFrame = 30;
    encode_param.qpmin = 10;
    encode_param.qpmax = 50;
    encode_param.bSvc = true;

    encode_param.encode_format = VENC_CODEC_H264;
    encode_param.encode_frame_num = 200;

    strcpy((char*)encode_param.intput_file,        "/data/camera/720p-30zhen.yuv");
    strcpy((char*)encode_param.output_file,        "/data/camera/720p.264");
    strcpy((char*)encode_param.reference_file,     "/mnt/bsp_ve_test/reference_data/reference.jpg");

    //parse the config paramter
    ParseArgument(&encode_param, argc, argv);
    printf(" input file: %s \n", encode_param.intput_file);
    printf(" output file: %s \n", encode_param.output_file);
    printf(" dst_size=%d, src_size=%d, bitrate=%d, qpmin=%d, qpmax=%d, framerate=%d\n", \
           encode_param.dst_size, encode_param.src_size,  encode_param.bit_rate, \
           encode_param.qpmin, encode_param.qpmax, encode_param.frame_rate);
    printf(" codec format : %d \n", encode_param.encode_format);
    printf(" get reference file: %s \n", encode_param.reference_file);

    // roi
    VencROIConfig sRoiConfig[4];

    sRoiConfig[0].bEnable = 1;
    sRoiConfig[0].index = 0;
    sRoiConfig[0].nQPoffset = 10;
    sRoiConfig[0].sRect.nLeft = 320;
    sRoiConfig[0].sRect.nTop = 180;
    sRoiConfig[0].sRect.nWidth = 320;
    sRoiConfig[0].sRect.nHeight = 180;

    sRoiConfig[1].bEnable = 1;
    sRoiConfig[1].index = 1;
    sRoiConfig[1].nQPoffset = 10;
    sRoiConfig[1].sRect.nLeft = 320;
    sRoiConfig[1].sRect.nTop = 180;
    sRoiConfig[1].sRect.nWidth = 320;
    sRoiConfig[1].sRect.nHeight = 180;

    sRoiConfig[2].bEnable = 1;
    sRoiConfig[2].index = 2;
    sRoiConfig[2].nQPoffset = 10;
    sRoiConfig[2].sRect.nLeft = 320;
    sRoiConfig[2].sRect.nTop = 180;
    sRoiConfig[2].sRect.nWidth = 320;
    sRoiConfig[2].sRect.nHeight = 180;

    sRoiConfig[3].bEnable = 1;
    sRoiConfig[3].index = 3;
    sRoiConfig[3].nQPoffset = 10;
    sRoiConfig[3].sRect.nLeft = 320;
    sRoiConfig[3].sRect.nTop = 180;
    sRoiConfig[3].sRect.nWidth = 320;
    sRoiConfig[3].sRect.nHeight = 180;

    //intraRefresh
    sIntraRefresh.bEnable = 1;
    sIntraRefresh.nBlockNumber = 10;

    //fix qp mode
    fixQP.bEnable = 1;
    fixQP.nIQp = 20;
    fixQP.nPQp = 30;

    //* h264 param
    h264Param.bEntropyCodingCABAC = 1;
    h264Param.nBitrate = encode_param.bit_rate;
    h264Param.nFramerate = encode_param.frame_rate;
    h264Param.nCodingMode = VENC_FRAME_CODING;
    //h264Param.nCodingMode = VENC_FIELD_CODING;

    h264Param.nMaxKeyInterval = encode_param.maxKeyFrame;
    h264Param.sProfileLevel.nProfile = VENC_H264ProfileHigh;
    h264Param.sProfileLevel.nLevel = VENC_H264Level51;
    h264Param.sQPRange.nMinqp = encode_param.qpmin;
    h264Param.sQPRange.nMaxqp = encode_param.qpmax;

	printf(" init jpeg\n");
    InitJpegExif(&exifinfo);

    input_path = encode_param.intput_file;
    output_path = encode_param.output_file;

    in_file = fopen(input_path, "r");
    if(in_file == NULL)
    {
        printf("open in_file fail\n");
        return -1;
    }

    out_file = fopen(output_path, "wb");
    if(out_file == NULL)
    {
        printf("open out_file fail\n");
        fclose(in_file);
        return -1;
    }

    if(encode_param.compare_flag)
    {
        reference_path = encode_param.reference_file;
        reference_file = fopen(reference_path, "r");
        if(reference_file == NULL)
        {
            printf("open reference_file fail\n");
            goto out;
        }

        reference_buffer = (unsigned char*)malloc(1*1024*1024);
        if(reference_buffer == NULL)
        {
            printf("malloc reference_buffer error\n");
            goto out;
        }
    }

    memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
    memset(&bufferParam, 0 ,sizeof(VencAllocateBufferParam));

    baseConfig.memops = MemAdapterGetOpsS();
    if (baseConfig.memops == NULL)
    {
        printf("MemAdapterGetOpsS failed\n");
        goto out;
    }
    CdcMemOpen(baseConfig.memops);
    baseConfig.nInputWidth= encode_param.src_width;
    baseConfig.nInputHeight = encode_param.src_height;
    baseConfig.nStride = encode_param.src_width;

    baseConfig.nDstWidth = encode_param.dst_width;
    baseConfig.nDstHeight = encode_param.dst_height;
    //the format of yuv file is yuv420p,
    //but the old ic only support the yuv420sp,
    //so use the func yu12_nv12() to config all the format.
    baseConfig.eInputFormat = VENC_PIXEL_YUV420SP;
	printf(" base config \n");

    int alignW = (baseConfig.nInputWidth + 15) & ~15;
    int alignH = (baseConfig.nInputHeight + 15) & ~15;

    bufferParam.nSizeY = alignW*alignH;
    bufferParam.nSizeC = alignW*alignH/2;
    bufferParam.nBufferNum = 4;

    pVideoEnc = VideoEncCreate(encode_param.encode_format);
    printf(" create video encoder \n" );

    if(encode_param.encode_format == VENC_CODEC_JPEG)
    {
        int quality = 90;
        int jpeg_mode = 1;
        VencJpegVideoSignal vs;
        vs.src_colour_primaries = VENC_YCC;
        vs.dst_colour_primaries = VENC_BT601;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegExifInfo, &exifinfo);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegQuality, &quality);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegEncMode, &jpeg_mode);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegVideoSignal, &vs);

        if(1 == jpeg_mode)
        {
            int jpeg_biteRate = 12*1024*1024;
            int jpeg_frameRate = 30;
            VencBitRateRange bitRateRange;
            bitRateRange.bitRateMax = 14*1024*1024;
            bitRateRange.bitRateMin = 10*1024*1024;

            VideoEncSetParameter(pVideoEnc, VENC_IndexParamBitrate, &jpeg_biteRate);
            VideoEncSetParameter(pVideoEnc, VENC_IndexParamFramerate, &jpeg_frameRate);
            VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetBitRateRange, &bitRateRange);
        }
    }
    else if(encode_param.encode_format == VENC_CODEC_H264)
    {
        int value;

        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264Param, &h264Param);
	printf(" set param h264 common, \n" );

    if(encode_param.bSvc){
        // Add for  Temporal SVC and Skip_Frame
        SVCSkip.nTemporalSVC = T_LAYER_3; //1080P support max T_LAYER_3 temporarily, fix it soon
        switch(SVCSkip.nTemporalSVC)
        {
            case T_LAYER_4:
                SVCSkip.nSkipFrame = SKIP_8;
                break;
            case T_LAYER_3:
                SVCSkip.nSkipFrame = SKIP_4;
                break;
            case T_LAYER_2:
                SVCSkip.nSkipFrame = SKIP_2;
                break;
            default:
                SVCSkip.nSkipFrame = NO_SKIP;
                break;
        }

        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264SVCSkip, &SVCSkip);
	    printf("set parm svc, temporal layer %d \n", SVCSkip.nTemporalSVC = T_LAYER_3);
    }

        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamIfilter, &value);

        value = 0; //degree
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamRotation, &value);

        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &fixQP);

        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264CyclicIntraRefresh, &sIntraRefresh);

        //value = 720/4;
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamSliceHeight, &value);

        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[0]);
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[1]);
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[2]);
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[3]);
        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetPSkip, &value);
#ifdef USE_ASPECT_RATIO
        sAspectRatio.aspect_ratio_idc = 255;
        sAspectRatio.sar_width = 4;
        sAspectRatio.sar_height = 3;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264AspectRatio, &sAspectRatio);
#endif
        //value = 1;
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FastEnc, &value);

#ifdef USE_VIDEO_SIGNAL
        sVideoSignal.video_format = 5;
        sVideoSignal.src_colour_primaries = 0;
        sVideoSignal.dst_colour_primaries = 1;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264VideoSignal, &sVideoSignal);
#endif

#ifdef USE_SUPER_FRAME
        //sSuperFrameCfg.eSuperFrameMode = VENC_SUPERFRAME_REENCODE;
        sSuperFrameCfg.eSuperFrameMode = VENC_SUPERFRAME_NONE;
        sSuperFrameCfg.nMaxIFrameBits = 30000*8;
        sSuperFrameCfg.nMaxPFrameBits = 15000*8;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSuperFrameConfig, &sSuperFrameCfg);
#endif
    }

    VideoEncInit(pVideoEnc, &baseConfig);

    if(encode_param.encode_format == VENC_CODEC_H264)
    {
        unsigned int head_num = 0;
        VideoEncGetParameter(pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
        encode_param.output_len += fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, out_file);
        printf(" sps_pps_data.nLength: %d, sps psp data   ", sps_pps_data.nLength);
        for(head_num=0; head_num<sps_pps_data.nLength; head_num++)
            printf("0x%02x  ", *(sps_pps_data.pBuffer+head_num));
	printf("\n");
    }

    AllocInputBuffer(pVideoEnc, &bufferParam);

    if(baseConfig.eInputFormat == VENC_PIXEL_YUV420SP)
    {
        uv_tmp_buffer = (unsigned char*)malloc(baseConfig.nInputWidth*baseConfig.nInputHeight/2);
        if(uv_tmp_buffer == NULL)
        {
            printf("malloc uv_tmp_buffer fail\n");
            fclose(out_file);
            fclose(in_file);
            return -1;
        }
    }

    unsigned int testNumber = 0;

    while(testNumber < encode_param.encode_frame_num)
    {
        GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);
        {
            unsigned int size1, size2;

            size1 = fread(inputBuffer.pAddrVirY, 1,
                    baseConfig.nInputWidth*baseConfig.nInputHeight, in_file);
            size2 = fread(inputBuffer.pAddrVirC, 1,
                      baseConfig.nInputWidth*baseConfig.nInputHeight/2, in_file);

            if((size1!= baseConfig.nInputWidth*baseConfig.nInputHeight)
             || (size2!= baseConfig.nInputWidth*baseConfig.nInputHeight/2))
            {
                fseek(in_file, 0L, SEEK_SET);
                size1 = fread(inputBuffer.pAddrVirY, 1,
                         baseConfig.nInputWidth*baseConfig.nInputHeight, in_file);
                size2 = fread(inputBuffer.pAddrVirC, 1,
                         baseConfig.nInputWidth*baseConfig.nInputHeight/2, in_file);
            }

            if(baseConfig.eInputFormat == VENC_PIXEL_YUV420SP)
            {
                yu12_nv12(baseConfig.nInputWidth, baseConfig.nInputHeight,
                     inputBuffer.pAddrVirC, uv_tmp_buffer);
            }
        }

        inputBuffer.bEnableCorp = 0;
        inputBuffer.sCropInfo.nLeft =  240;
        inputBuffer.sCropInfo.nTop  =  240;
        inputBuffer.sCropInfo.nWidth  =  240;
        inputBuffer.sCropInfo.nHeight =  240;

        FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);

        //pts += 66000;
        //inputBuffer.nPts = pts;

        AddOneInputBuffer(pVideoEnc, &inputBuffer);
        time1 = GetNowUs();
        VideoEncodeOneFrame(pVideoEnc);
        time2 = GetNowUs();
        time3 += time2-time1;
        printf(" encode frame %d use time is %lldus..\r",testNumber,(time2-time1));

        AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);
        ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);

        result = GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
#ifdef USE_SUPER_FRAME
        if((sSuperFrameCfg.eSuperFrameMode==VENC_SUPERFRAME_DISCARD) && (result==-1))
        {
            printf(" VENC_SUPERFRAME_DISCARD: discard frame %d\n",testNumber);
            continue;
        }
#endif
        if(result == -1)
        {
            goto out;
        }

        encode_param.output_len += fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);

        if(outputBuffer.nSize1)
        {
            encode_param.output_len += fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
        }

        if(encode_param.compare_flag)
        {
            result = fread(reference_buffer, 1, outputBuffer.nSize0, reference_file);
            if(result != outputBuffer.nSize0)
            {
                printf("read reference_file error\n");
                goto out;
            }

            for(i=0; i<outputBuffer.nSize0; i++)
            {
                if((outputBuffer.pData0)[i] != reference_buffer[i])
                {
                    printf("the %d frame's data0 %d byte is not same\n", testNumber, i);
                    goto out;
                }
            }

            result = fread(reference_buffer, 1, outputBuffer.nSize1, reference_file);
            if(result != outputBuffer.nSize1)
            {
                printf("read reference_file error\n");
                goto out;
            }

            for(i=0; i<outputBuffer.nSize1; i++)
            {
                if((outputBuffer.pData1)[i] != reference_buffer[i])
                {
                    printf("the %d frame's data1 %d byte is not same\n", testNumber, i);
                    goto out;
                }
            }
        }

        FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);

        if(h264Param.nCodingMode==VENC_FIELD_CODING && encode_param.encode_format==VENC_CODEC_H264)
        {
            GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
            //logi("size: %d,%d", outputBuffer.nSize0,outputBuffer.nSize1);

            fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);

            if(outputBuffer.nSize1)
            {
                fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
            }

            FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);
        }

        testNumber++;
    }

    if(encode_param.compare_flag)
    {
        encode_param.compare_result = 1;
        printf("the compare result is ok\n");
    }

    printf("\n the average encode time is %lldus, fps= %lld ...\n",time3/testNumber, (1000000/(time3/testNumber)));
    if(pVideoEnc)
    {
        VideoEncDestroy(pVideoEnc);
    }
    pVideoEnc = NULL;
    printf(" output file size is %d, encode_time= %lldus, expect_bitrate= %dbps, real_bitrate= %fbps\n",encode_param.output_len, time3,encode_param.bit_rate, (encode_param.output_len*1000000)/time3);

out:
    if(out_file)
        fclose(out_file);
    if(in_file)
        fclose(in_file);
    if(uv_tmp_buffer)
        free(uv_tmp_buffer);
    if(baseConfig.memops)
    {
        CdcMemClose(baseConfig.memops);
    }

    if(encode_param.compare_flag)
    {
        if(reference_buffer)
            free(reference_buffer);
        if(reference_file)
            fclose(reference_file);

        if(encode_param.compare_result)
        {
            printf("****************************************\n");
            printf("the compare result is ok\n");
            printf("the compare result is ok\n");
            printf("the compare result is ok\n");
            printf("****************************************\n");
        }
        else
        {
            printf("****************************************\n");
            printf("the compare result is fail\n");
            printf("the compare result is fail\n");
            printf("the compare result is fail\n");
            printf("****************************************\n");
            return -1;
        }
    }

    return 0;
}
