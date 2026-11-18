#include "webcam.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static unsigned char ClampByte(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

static void YuvToRgbPixel(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b) {
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;
    if (c < 0) c = 0;
    *r = ClampByte((298 * c + 409 * e + 128) >> 8);
    *g = ClampByte((298 * c - 100 * d - 208 * e + 128) >> 8);
    *b = ClampByte((298 * c + 516 * d + 128) >> 8);
}

static void CropMirrorYUYVToRGB(const unsigned char *src, int srcW, int srcH, int stride,
                                unsigned char *dst, int dstW, int dstH) {
    float aspectDst = (float)dstW / (float)dstH;
    float aspectSrc = (float)srcW / (float)srcH;
    int cropW = srcW, cropH = srcH, cropX = 0, cropY = 0;
    if (aspectSrc > aspectDst) {
        cropW = (int)(srcH * aspectDst);
        cropX = (srcW - cropW) / 2;
    } else {
        cropH = (int)(srcW / aspectDst);
        cropY = (srcH - cropH) / 2;
    }
    if (cropW < 1) cropW = 1;
    if (cropH < 1) cropH = 1;

    for (int dy = 0; dy < dstH; dy++) {
        int sy = cropY + (dy * cropH) / dstH;
        if (sy < 0) sy = 0;
        if (sy >= srcH) sy = srcH - 1;
        const unsigned char *row = src + sy * stride;

        for (int dx = 0; dx < dstW; dx++) {
            // Espelhamento horizontal para funcionar como espelho natural ao posar
            int sx = cropX + cropW - 1 - (dx * cropW) / dstW;
            if (sx < 0) sx = 0;
            if (sx >= srcW) sx = srcW - 1;

            int pairX = sx & ~1;
            const unsigned char *p = row + pairX * 2;
            int yVal = (sx & 1) ? p[2] : p[0];
            int uVal = p[1];
            int vVal = p[3];

            unsigned char *out = dst + (dy * dstW + dx) * 3;
            YuvToRgbPixel(yVal, uVal, vVal, &out[0], &out[1], &out[2]);
        }
    }
}

#if defined(__linux__)

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define MAX_V4L2_BUFS 4

typedef struct {
    void  *start;
    size_t length;
} V4l2Buf;

static int          g_fd       = -1;
static V4l2Buf      g_bufs[MAX_V4L2_BUFS];
static unsigned int g_numBufs  = 0;
static int          g_camW     = 0;
static int          g_camH     = 0;
static int          g_stride   = 0;
static unsigned int g_pixFmt   = 0;
static bool         g_streaming = false;

static int Xioctl(int fd, unsigned long request, void *arg) {
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

bool WebcamOpen(void) {
    if (g_fd >= 0) return true;

    for (int devIdx = 0; devIdx < 6; devIdx++) {
        char path[32];
        snprintf(path, sizeof path, "/dev/video%d", devIdx);
        int fd = open(path, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) continue;

        struct v4l2_capability cap;
        memset(&cap, 0, sizeof cap);
        if (Xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
            close(fd);
            continue;
        }
        unsigned int caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
        if (!(caps & V4L2_CAP_VIDEO_CAPTURE) || !(caps & V4L2_CAP_STREAMING)) {
            close(fd);
            continue;
        }

        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof fmt);
        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_ANY;

        if (Xioctl(fd, VIDIOC_S_FMT, &fmt) < 0 ||
            (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV &&
             fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24)) {
            close(fd);
            continue;
        }

        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof req);
        req.count  = MAX_V4L2_BUFS;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (Xioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
            close(fd);
            continue;
        }

        memset(g_bufs, 0, sizeof g_bufs);
        g_numBufs = (req.count > MAX_V4L2_BUFS) ? MAX_V4L2_BUFS : req.count;
        bool mmapOk = true;
        for (unsigned int i = 0; i < g_numBufs; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof buf);
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;
            if (Xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) { mmapOk = false; break; }
            g_bufs[i].length = buf.length;
            g_bufs[i].start  = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            if (g_bufs[i].start == MAP_FAILED) { g_bufs[i].start = NULL; mmapOk = false; break; }
        }
        if (!mmapOk) {
            for (unsigned int i = 0; i < g_numBufs; i++) {
                if (g_bufs[i].start) munmap(g_bufs[i].start, g_bufs[i].length);
            }
            close(fd);
            continue;
        }

        for (unsigned int i = 0; i < g_numBufs; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof buf);
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;
            if (Xioctl(fd, VIDIOC_QBUF, &buf) < 0) { mmapOk = false; break; }
        }
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (!mmapOk || Xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            for (unsigned int i = 0; i < g_numBufs; i++) {
                if (g_bufs[i].start) munmap(g_bufs[i].start, g_bufs[i].length);
            }
            close(fd);
            continue;
        }

        g_fd        = fd;
        g_camW      = (int)fmt.fmt.pix.width;
        g_camH      = (int)fmt.fmt.pix.height;
        g_stride    = (int)fmt.fmt.pix.bytesperline;
        if (g_stride <= 0) g_stride = g_camW * ((fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) ? 3 : 2);
        g_pixFmt    = fmt.fmt.pix.pixelformat;
        g_streaming = true;
        return true;
    }
    return false;
}

void WebcamClose(void) {
    if (g_fd < 0) return;
    if (g_streaming) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        Xioctl(g_fd, VIDIOC_STREAMOFF, &type);
        g_streaming = false;
    }
    for (unsigned int i = 0; i < g_numBufs; i++) {
        if (g_bufs[i].start) {
            munmap(g_bufs[i].start, g_bufs[i].length);
            g_bufs[i].start = NULL;
        }
    }
    g_numBufs = 0;
    close(g_fd);
    g_fd = -1;
}

bool WebcamIsOpen(void) {
    return g_fd >= 0 && g_streaming;
}

bool WebcamGrabFrameRGB(unsigned char *outRgb, int outW, int outH) {
    if (!WebcamIsOpen() || !outRgb || outW <= 0 || outH <= 0) return false;

    int ultimoIdx = -1;
    struct v4l2_buffer ultimoBuf;
    memset(&ultimoBuf, 0, sizeof ultimoBuf);

    // Drena todos os quadros prontos na fila para pegar sempre o quadro mais recente sem atraso
    for (unsigned int tent = 0; tent < g_numBufs; tent++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (Xioctl(g_fd, VIDIOC_DQBUF, &buf) < 0) {
            break;
        }
        if (ultimoIdx >= 0) {
            Xioctl(g_fd, VIDIOC_QBUF, &ultimoBuf);
        }
        ultimoIdx = (int)buf.index;
        ultimoBuf = buf;
    }

    if (ultimoIdx < 0) return false;

    const unsigned char *src = (const unsigned char *)g_bufs[ultimoIdx].start;
    if (g_pixFmt == V4L2_PIX_FMT_YUYV) {
        CropMirrorYUYVToRGB(src, g_camW, g_camH, g_stride, outRgb, outW, outH);
    } else if (g_pixFmt == V4L2_PIX_FMT_RGB24) {
        float aspectDst = (float)outW / (float)outH;
        float aspectSrc = (float)g_camW / (float)g_camH;
        int cropW = g_camW, cropH = g_camH, cropX = 0, cropY = 0;
        if (aspectSrc > aspectDst) { cropW = (int)(g_camH * aspectDst); cropX = (g_camW - cropW) / 2; }
        else                       { cropH = (int)(g_camW / aspectDst); cropY = (g_camH - cropH) / 2; }
        for (int dy = 0; dy < outH; dy++) {
            int sy = cropY + (dy * cropH) / outH;
            const unsigned char *row = src + sy * g_stride;
            for (int dx = 0; dx < outW; dx++) {
                int sx = cropX + cropW - 1 - (dx * cropW) / outW;
                const unsigned char *p = row + sx * 3;
                unsigned char *o = outRgb + (dy * outW + dx) * 3;
                o[0] = p[0]; o[1] = p[1]; o[2] = p[2];
            }
        }
    }

    Xioctl(g_fd, VIDIOC_QBUF, &ultimoBuf);
    return true;
}

#elif defined(_WIN32)

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

// GUID de MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING declarado estaticamente para compilar em qualquer SDK Windows (MSVC/MinGW)
static const GUID kMfEnableVideoProcessing = {
    0xfb394f3d, 0xccf1, 0x42ee, { 0xbb, 0xb3, 0xf9, 0xb8, 0x45, 0xd5, 0x68, 0x1d }
};
#define MF_VIDPROC_GUID (&kMfEnableVideoProcessing)

static IMFSourceReader *g_reader     = NULL;
static HANDLE           g_thread     = NULL;
static CRITICAL_SECTION g_cs;
static volatile LONG    g_running    = 0;
static bool             g_open       = false;
static bool             g_comInit    = false;
static bool             g_mfInit     = false;
static unsigned int     g_camW       = 0;
static unsigned int     g_camH       = 0;
static LONG             g_stride     = 0;
static bool             g_isYuy2     = false;
static unsigned char   *g_rawLatest  = NULL;
static size_t           g_rawCap     = 0;
static size_t           g_rawLen     = 0;
static bool             g_hasNewRaw  = false;

static DWORD WINAPI WinCamThreadProc(LPVOID param) {
    (void)param;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        DWORD streamIdx = 0, flags = 0;
        LONGLONG ts = 0;
        IMFSample *sample = NULL;
        HRESULT hr = IMFSourceReader_ReadSample(
            g_reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
            &streamIdx, &flags, &ts, &sample
        );
        if (SUCCEEDED(hr) && sample != NULL) {
            IMFMediaBuffer *mbuf = NULL;
            if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(sample, &mbuf)) && mbuf != NULL) {
                BYTE *ptr = NULL;
                DWORD maxLen = 0, curLen = 0;
                if (SUCCEEDED(IMFMediaBuffer_Lock(mbuf, &ptr, &maxLen, &curLen)) && ptr != NULL && curLen > 0) {
                    EnterCriticalSection(&g_cs);
                    if (curLen > g_rawCap) {
                        unsigned char *nb = (unsigned char *)realloc(g_rawLatest, curLen);
                        if (nb) { g_rawLatest = nb; g_rawCap = curLen; }
                    }
                    if (g_rawLatest && curLen <= g_rawCap) {
                        memcpy(g_rawLatest, ptr, curLen);
                        g_rawLen = curLen;
                        g_hasNewRaw = true;
                    }
                    LeaveCriticalSection(&g_cs);
                    IMFMediaBuffer_Unlock(mbuf);
                }
                IMFMediaBuffer_Release(mbuf);
            }
            IMFSample_Release(sample);
        } else {
            Sleep(8);
        }
    }
    CoUninitialize();
    return 0;
}

bool WebcamOpen(void) {
    if (g_open) return true;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    g_comInit = SUCCEEDED(hr);
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET))) {
        if (g_comInit) { CoUninitialize(); g_comInit = false; }
        return false;
    }
    g_mfInit = true;

    IMFAttributes *attr = NULL;
    IMFActivate **devices = NULL;
    UINT32 count = 0;
    if (FAILED(MFCreateAttributes(&attr, 1))) { WebcamClose(); return false; }
    if (FAILED(IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                     &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) ||
        FAILED(MFEnumDeviceSources(attr, &devices, &count)) || count == 0) {
        IMFAttributes_Release(attr);
        WebcamClose();
        return false;
    }
    IMFAttributes_Release(attr);

    IMFMediaSource *source = NULL;
    hr = IMFActivate_ActivateObject(devices[0], &IID_IMFMediaSource, (void **)&source);
    for (UINT32 i = 0; i < count; i++) IMFActivate_Release(devices[i]);
    CoTaskMemFree(devices);
    if (FAILED(hr) || !source) { WebcamClose(); return false; }

    IMFAttributes *rdAttr = NULL;
    if (SUCCEEDED(MFCreateAttributes(&rdAttr, 1))) {
        IMFAttributes_SetUINT32(rdAttr, MF_VIDPROC_GUID, TRUE);
    }
    hr = MFCreateSourceReaderFromMediaSource(source, rdAttr, &g_reader);
    if (rdAttr) IMFAttributes_Release(rdAttr);
    IMFMediaSource_Release(source);
    if (FAILED(hr) || !g_reader) { WebcamClose(); return false; }

    // Tenta configurar saida RGB32 (com conversao automatica do Media Foundation) ou YUY2 como fallback
    IMFMediaType *mt = NULL;
    bool okFmt = false;
    g_isYuy2 = false;
    if (SUCCEEDED(MFCreateMediaType(&mt))) {
        IMFMediaType_SetGUID(mt, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        IMFMediaType_SetGUID(mt, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
        if (SUCCEEDED(IMFSourceReader_SetCurrentMediaType(g_reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mt))) {
            okFmt = true;
        } else {
            IMFMediaType_SetGUID(mt, &MF_MT_SUBTYPE, &MFVideoFormat_YUY2);
            if (SUCCEEDED(IMFSourceReader_SetCurrentMediaType(g_reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mt))) {
                okFmt = true;
                g_isYuy2 = true;
            }
        }
        IMFMediaType_Release(mt);
    }
    if (!okFmt) { WebcamClose(); return false; }

    IMFMediaType *curMt = NULL;
    if (FAILED(IMFSourceReader_GetCurrentMediaType(g_reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &curMt)) || !curMt) {
        WebcamClose();
        return false;
    }
    UINT64 frameSize = 0;
    IMFMediaType_GetUINT64(curMt, &MF_MT_FRAME_SIZE, &frameSize);
    g_camW = (unsigned int)(frameSize >> 32);
    g_camH = (unsigned int)(frameSize & 0xFFFFFFFFu);
    UINT32 strideU = 0;
    if (SUCCEEDED(IMFMediaType_GetUINT32(curMt, &MF_MT_DEFAULT_STRIDE, &strideU))) {
        g_stride = (LONG)strideU;
    } else {
        g_stride = (LONG)(g_camW * (g_isYuy2 ? 2u : 4u));
    }
    IMFMediaType_Release(curMt);
    if (g_camW == 0 || g_camH == 0) { WebcamClose(); return false; }

    InitializeCriticalSection(&g_cs);
    g_hasNewRaw = false;
    g_running = 1;
    g_thread = CreateThread(NULL, 0, WinCamThreadProc, NULL, 0, NULL);
    if (!g_thread) {
        DeleteCriticalSection(&g_cs);
        WebcamClose();
        return false;
    }
    g_open = true;
    return true;
}

void WebcamClose(void) {
    if (g_thread) {
        InterlockedExchange(&g_running, 0);
        WaitForSingleObject(g_thread, 1500);
        CloseHandle(g_thread);
        g_thread = NULL;
        DeleteCriticalSection(&g_cs);
    }
    if (g_rawLatest) {
        free(g_rawLatest);
        g_rawLatest = NULL;
        g_rawCap = 0;
    }
    if (g_reader) {
        IMFSourceReader_Release(g_reader);
        g_reader = NULL;
    }
    if (g_mfInit) {
        MFShutdown();
        g_mfInit = false;
    }
    if (g_comInit) {
        CoUninitialize();
        g_comInit = false;
    }
    g_open = false;
}

bool WebcamIsOpen(void) {
    return g_open;
}

bool WebcamGrabFrameRGB(unsigned char *outRgb, int outW, int outH) {
    if (!g_open || !outRgb || outW <= 0 || outH <= 0) return false;

    bool got = false;
    EnterCriticalSection(&g_cs);
    if (g_hasNewRaw && g_rawLatest) {
        int absStride = (g_stride < 0) ? (int)(-g_stride) : (int)g_stride;
        bool bottomUp = (g_stride < 0);
        int sw = (int)g_camW;
        int sh = (int)g_camH;

        if (g_isYuy2) {
            CropMirrorYUYVToRGB(g_rawLatest, sw, sh, absStride, outRgb, outW, outH);
        } else {
            // MFVideoFormat_RGB32 (BGRA em memoria)
            float aspectDst = (float)outW / (float)outH;
            float aspectSrc = (float)sw / (float)sh;
            int cropW = sw, cropH = sh, cropX = 0, cropY = 0;
            if (aspectSrc > aspectDst) { cropW = (int)(sh * aspectDst); cropX = (sw - cropW) / 2; }
            else                       { cropH = (int)(sw / aspectDst); cropY = (sh - cropH) / 2; }
            for (int dy = 0; dy < outH; dy++) {
                int sy = cropY + (dy * cropH) / outH;
                int memY = bottomUp ? (sh - 1 - sy) : sy;
                size_t rowOff = (size_t)memY * (size_t)absStride;
                for (int dx = 0; dx < outW; dx++) {
                    int sx = cropX + cropW - 1 - (dx * cropW) / outW;
                    unsigned char *o = outRgb + (dy * outW + dx) * 3;
                    if (rowOff + (size_t)(sx + 1) * 4u <= g_rawLen) {
                        const unsigned char *p = g_rawLatest + rowOff + (size_t)sx * 4u;
                        o[0] = p[2]; // R
                        o[1] = p[1]; // G
                        o[2] = p[0]; // B
                    } else {
                        o[0] = o[1] = o[2] = 0;
                    }
                }
            }
        }
        g_hasNewRaw = false;
        got = true;
    }
    LeaveCriticalSection(&g_cs);
    return got;
}

#else

bool WebcamOpen(void) { return false; }
void WebcamClose(void) { }
bool WebcamIsOpen(void) { return false; }
bool WebcamGrabFrameRGB(unsigned char *outRgb, int outW, int outH) {
    (void)outRgb; (void)outW; (void)outH;
    return false;
}

#endif
