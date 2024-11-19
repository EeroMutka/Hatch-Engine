
// NOTE (Eero Mutka, 2024): I made some modifications to this file, primarily adding the DDSPP_ prefix to all names and marking everything static.

/*
MIT License

Copyright (c) 2018-2023 Emilio López

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Sources
// https://learn.microsoft.com/en-us/windows/uwp/gaming/complete-code-for-ddstextureloader
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat

static const unsigned int DDSPP_DDS_MAGIC      = 0x20534444;

static const unsigned int DDSPP_DDS_ALPHAPIXELS = 0x00000001;
static const unsigned int DDSPP_DDS_ALPHA       = 0x00000002; // DDPF_ALPHA
static const unsigned int DDSPP_DDS_FOURCC      = 0x00000004; // DDPF_FOURCC
static const unsigned int DDSPP_DDS_RGB         = 0x00000040; // DDPF_RGB
static const unsigned int DDSPP_DDS_RGBA        = 0x00000041; // DDPF_RGB | DDPF_ALPHAPIXELS
static const unsigned int DDSPP_DDS_YUV         = 0x00000200; // DDPF_YUV
static const unsigned int DDSPP_DDS_LUMINANCE   = 0x00020000; // DDPF_LUMINANCE
static const unsigned int DDSPP_DDS_LUMINANCEA  = 0x00020001; // DDPF_LUMINANCE | DDPF_ALPHAPIXELS

static const unsigned int DDSPP_DDS_PAL8       = 0x00000020; // DDPF_PALETTEINDEXED8
static const unsigned int DDSPP_DDS_BUMPDUDV   = 0x00080000; // DDPF_BUMPDUDV

static const unsigned int DDSPP_DDS_HEADER_FLAGS_CAPS        = 0x00000001; // DDSD_CAPS
static const unsigned int DDSPP_DDS_HEADER_FLAGS_HEIGHT      = 0x00000002; // DDSD_HEIGHT
static const unsigned int DDSPP_DDS_HEADER_FLAGS_WIDTH       = 0x00000004; // DDSD_WIDTH
static const unsigned int DDSPP_DDS_HEADER_FLAGS_PITCH       = 0x00000008; // DDSD_PITCH
static const unsigned int DDSPP_DDS_HEADER_FLAGS_PIXELFORMAT = 0x00001000; // DDSD_PIXELFORMAT
static const unsigned int DDSPP_DDS_HEADER_FLAGS_MIPMAP      = 0x00020000; // DDSD_MIPMAPCOUNT
static const unsigned int DDSPP_DDS_HEADER_FLAGS_LINEARSIZE  = 0x00080000; // DDSD_LINEARSIZE
static const unsigned int DDSPP_DDS_HEADER_FLAGS_VOLUME      = 0x00800000; // DDSD_DEPTH

static const unsigned int DDSPP_DDS_HEADER_CAPS_COMPLEX      = 0x00000008; // DDSCAPS_COMPLEX
static const unsigned int DDSPP_DDS_HEADER_CAPS_MIPMAP       = 0x00400000; // DDSCAPS_MIPMAP
static const unsigned int DDSPP_DDS_HEADER_CAPS_TEXTURE      = 0x00001000; // DDSCAPS_TEXTURE

static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP           = 0x00000200; // DDSCAPS2_CUBEMAP
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEX = 0x00000600; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEX = 0x00000a00; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEY = 0x00001200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEY = 0x00002200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEZ = 0x00004200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
static const unsigned int DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEZ = 0x00008200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ
static const unsigned int DDSPP_DDS_HEADER_CAPS2_VOLUME            = 0x00200000; // DDSCAPS2_VOLUME
#define DDSPP_DDS_HEADER_CAPS2_CUBEMAP_ALLFACES  (DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEX | DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEX | DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEY |  DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEY | DDSPP_DDS_HEADER_CAPS2_CUBEMAP_POSITIVEZ | DDSPP_DDS_HEADER_CAPS2_CUBEMAP_NEGATIVEZ)

static const unsigned int DDSPP_DXGI_MISC_FLAG_CUBEMAP = 0x2; // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag
static const unsigned int DDSPP_DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7;

#define DDSPP_MakeFourCC(a, b, c, d) ((a) + ((b) << 8) + ((c) << 16) + ((d) << 24))

// FOURCC constants
#define DDSPP_FOURCC_DXT1    DDSPP_MakeFourCC('D', 'X', 'T', '1') // DDSPP_FORMAT_BC1_UNORM
#define DDSPP_FOURCC_DXT2    DDSPP_MakeFourCC('D', 'X', 'T', '2') // DDSPP_FORMAT_BC2_UNORM
#define DDSPP_FOURCC_DXT3    DDSPP_MakeFourCC('D', 'X', 'T', '3') // DDSPP_FORMAT_BC2_UNORM
#define DDSPP_FOURCC_DXT4    DDSPP_MakeFourCC('D', 'X', 'T', '4') // DDSPP_FORMAT_BC3_UNORM
#define DDSPP_FOURCC_DXT5    DDSPP_MakeFourCC('D', 'X', 'T', '5') // DDSPP_FORMAT_BC3_UNORM
#define DDSPP_FOURCC_ATI1    DDSPP_MakeFourCC('A', 'T', 'I', '1') // DDSPP_FORMAT_BC4_UNORM
#define DDSPP_FOURCC_BC4U    DDSPP_MakeFourCC('B', 'C', '4', 'U') // DDSPP_FORMAT_BC4_UNORM
#define DDSPP_FOURCC_BC4S    DDSPP_MakeFourCC('B', 'C', '4', 'S') // DDSPP_FORMAT_BC4_SNORM
#define DDSPP_FOURCC_ATI2    DDSPP_MakeFourCC('A', 'T', 'I', '2') // DDSPP_FORMAT_BC5_UNORM
#define DDSPP_FOURCC_BC5U    DDSPP_MakeFourCC('B', 'C', '5', 'U') // DDSPP_FORMAT_BC5_UNORM
#define DDSPP_FOURCC_BC5S    DDSPP_MakeFourCC('B', 'C', '5', 'S') // DDSPP_FORMAT_BC5_SNORM
#define DDSPP_FOURCC_RGBG    DDSPP_MakeFourCC('R', 'G', 'B', 'G') // DDSPP_FORMAT_R8G8_B8G8_UNORM
#define DDSPP_FOURCC_GRBG    DDSPP_MakeFourCC('G', 'R', 'G', 'B') // DDSPP_FORMAT_G8R8_G8B8_UNORM
#define DDSPP_FOURCC_YUY2    DDSPP_MakeFourCC('Y', 'U', 'Y', '2') // DDSPP_FORMAT_YUY2
#define DDSPP_FOURCC_DXT10   DDSPP_MakeFourCC('D', 'X', '1', '0') // DDS extension header

// These values come from the original D3D9 D3DFORMAT values https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat
#define DDSPP_FOURCC_RGB8          20
#define DDSPP_FOURCC_A8R8G8B8      21
#define DDSPP_FOURCC_X8R8G8B8      22
#define DDSPP_FOURCC_R5G6B5        23 // DDSPP_FORMAT_B5G6R5_UNORM   (needs swizzling)
#define DDSPP_FOURCC_X1R5G5B5      24
#define DDSPP_FOURCC_RGB5A1        25 // DDSPP_FORMAT_B5G5R5A1_UNORM (needs swizzling)
#define DDSPP_FOURCC_RGBA4         26 // DDSPP_FORMAT_B4G4R4A4_UNORM (needs swizzling)
#define DDSPP_FOURCC_R3G3B2        27
#define DDSPP_FOURCC_A8            28
#define DDSPP_FOURCC_A8R3G3B2      29
#define DDSPP_FOURCC_X4R4G4B4      30
#define DDSPP_FOURCC_A2B10G10R10   31
#define DDSPP_FOURCC_A8B8G8R8      32
#define DDSPP_FOURCC_X8B8G8R8      33
#define DDSPP_FOURCC_G16R16        34
#define DDSPP_FOURCC_A2R10G10B10   35
#define DDSPP_FOURCC_RGBA16U       36 // DDSPP_FORMAT_R16G16B16A16_UNORM
#define DDSPP_FOURCC_RGBA16S      110 // DDSPP_FORMAT_R16G16B16A16_SNORM
#define DDSPP_FOURCC_R16F         111 // DDSPP_FORMAT_R16_FLOAT
#define DDSPP_FOURCC_RG16F        112 // DDSPP_FORMAT_R16G16_FLOAT
#define DDSPP_FOURCC_RGBA16F      113 // DDSPP_FORMAT_R16G16B16A16_FLOAT
#define DDSPP_FOURCC_R32F         114 // DDSPP_FORMAT_R32_FLOAT
#define DDSPP_FOURCC_RG32F        115 // DDSPP_FORMAT_R32G32_FLOAT
#define DDSPP_FOURCC_RGBA32F      116 // DDSPP_FORMAT_R32G32B32A32_FLOAT

typedef struct {
	unsigned int size;
	unsigned int flags;
	unsigned int fourCC;
	unsigned int RGBBitCount;
	unsigned int RBitMask;
	unsigned int GBitMask;
	unsigned int BBitMask;
	unsigned int ABitMask;
} DDSPP_PixelFormat;

// static_assert(sizeof(DDSPP_PixelFormat) == 32, "DDSPP_PixelFormat size mismatch");

static inline const bool DDSPP_IsRGBAMask(const DDSPP_PixelFormat* ddspf, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask) {
	return (ddspf->RBitMask == rmask) && (ddspf->GBitMask == gmask) && (ddspf->BBitMask == bmask) && (ddspf->ABitMask == amask);
}

static inline const bool DDSPP_IsRGBMask(const DDSPP_PixelFormat* ddspf, unsigned int rmask, unsigned int gmask, unsigned int bmask) {
	return (ddspf->RBitMask == rmask) && (ddspf->GBitMask == gmask) && (ddspf->BBitMask == bmask);
}

// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ne-d3d11-d3d11_resource_dimension
typedef enum {
	DXGI_Unknown,
	DXGI_Buffer,
	DXGI_Texture1D,
	DXGI_Texture2D,
	DXGI_Texture3D
} DDSPP_DXGIResourceDimension;

// Matches DXGI_FORMAT https://docs.microsoft.com/en-us/windows/desktop/api/dxgiformat/ne-dxgiformat-dxgi_format
typedef enum {
	DDSPP_FORMAT_UNKNOWN	                    = 0,
	DDSPP_FORMAT_R32G32B32A32_TYPELESS       = 1,
	DDSPP_FORMAT_R32G32B32A32_FLOAT          = 2,
	DDSPP_FORMAT_R32G32B32A32_UINT           = 3,
	DDSPP_FORMAT_R32G32B32A32_SINT           = 4,
	DDSPP_FORMAT_R32G32B32_TYPELESS          = 5,
	DDSPP_FORMAT_R32G32B32_FLOAT             = 6,
	DDSPP_FORMAT_R32G32B32_UINT              = 7,
	DDSPP_FORMAT_R32G32B32_SINT              = 8,
	DDSPP_FORMAT_R16G16B16A16_TYPELESS       = 9,
	DDSPP_FORMAT_R16G16B16A16_FLOAT          = 10,
	DDSPP_FORMAT_R16G16B16A16_UNORM          = 11,
	DDSPP_FORMAT_R16G16B16A16_UINT           = 12,
	DDSPP_FORMAT_R16G16B16A16_SNORM          = 13,
	DDSPP_FORMAT_R16G16B16A16_SINT           = 14,
	DDSPP_FORMAT_R32G32_TYPELESS             = 15,
	DDSPP_FORMAT_R32G32_FLOAT                = 16,
	DDSPP_FORMAT_R32G32_UINT                 = 17,
	DDSPP_FORMAT_R32G32_SINT                 = 18,
	DDSPP_FORMAT_R32G8X24_TYPELESS           = 19,
	DDSPP_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	DDSPP_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
	DDSPP_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
	DDSPP_FORMAT_R10G10B10A2_TYPELESS        = 23,
	DDSPP_FORMAT_R10G10B10A2_UNORM           = 24,
	DDSPP_FORMAT_R10G10B10A2_UINT            = 25,
	DDSPP_FORMAT_R11G11B10_FLOAT             = 26,
	DDSPP_FORMAT_R8G8B8A8_TYPELESS           = 27,
	DDSPP_FORMAT_R8G8B8A8_UNORM              = 28,
	DDSPP_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	DDSPP_FORMAT_R8G8B8A8_UINT               = 30,
	DDSPP_FORMAT_R8G8B8A8_SNORM              = 31,
	DDSPP_FORMAT_R8G8B8A8_SINT               = 32,
	DDSPP_FORMAT_R16G16_TYPELESS             = 33,
	DDSPP_FORMAT_R16G16_FLOAT                = 34,
	DDSPP_FORMAT_R16G16_UNORM                = 35,
	DDSPP_FORMAT_R16G16_UINT                 = 36,
	DDSPP_FORMAT_R16G16_SNORM                = 37,
	DDSPP_FORMAT_R16G16_SINT                 = 38,
	DDSPP_FORMAT_R32_TYPELESS                = 39,
	DDSPP_FORMAT_D32_FLOAT                   = 40,
	DDSPP_FORMAT_R32_FLOAT                   = 41,
	DDSPP_FORMAT_R32_UINT                    = 42,
	DDSPP_FORMAT_R32_SINT                    = 43,
	DDSPP_FORMAT_R24G8_TYPELESS              = 44,
	DDSPP_FORMAT_D24_UNORM_S8_UINT           = 45,
	DDSPP_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
	DDSPP_FORMAT_X24_TYPELESS_G8_UINT        = 47,
	DDSPP_FORMAT_R8G8_TYPELESS               = 48,
	DDSPP_FORMAT_R8G8_UNORM                  = 49,
	DDSPP_FORMAT_R8G8_UINT                   = 50,
	DDSPP_FORMAT_R8G8_SNORM                  = 51,
	DDSPP_FORMAT_R8G8_SINT                   = 52,
	DDSPP_FORMAT_R16_TYPELESS                = 53,
	DDSPP_FORMAT_R16_FLOAT                   = 54,
	DDSPP_FORMAT_D16_UNORM                   = 55,
	DDSPP_FORMAT_R16_UNORM                   = 56,
	DDSPP_FORMAT_R16_UINT                    = 57,
	DDSPP_FORMAT_R16_SNORM                   = 58,
	DDSPP_FORMAT_R16_SINT                    = 59,
	DDSPP_FORMAT_R8_TYPELESS                 = 60,
	DDSPP_FORMAT_R8_UNORM                    = 61,
	DDSPP_FORMAT_R8_UINT                     = 62,
	DDSPP_FORMAT_R8_SNORM                    = 63,
	DDSPP_FORMAT_R8_SINT                     = 64,
	DDSPP_FORMAT_A8_UNORM                    = 65,
	DDSPP_FORMAT_R1_UNORM                    = 66,
	DDSPP_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
	DDSPP_FORMAT_R8G8_B8G8_UNORM             = 68,
	DDSPP_FORMAT_G8R8_G8B8_UNORM             = 69,
	DDSPP_FORMAT_BC1_TYPELESS                = 70,
	DDSPP_FORMAT_BC1_UNORM                   = 71,
	DDSPP_FORMAT_BC1_UNORM_SRGB              = 72,
	DDSPP_FORMAT_BC2_TYPELESS                = 73,
	DDSPP_FORMAT_BC2_UNORM                   = 74,
	DDSPP_FORMAT_BC2_UNORM_SRGB              = 75,
	DDSPP_FORMAT_BC3_TYPELESS                = 76,
	DDSPP_FORMAT_BC3_UNORM                   = 77,
	DDSPP_FORMAT_BC3_UNORM_SRGB              = 78,
	DDSPP_FORMAT_BC4_TYPELESS                = 79,
	DDSPP_FORMAT_BC4_UNORM                   = 80,
	DDSPP_FORMAT_BC4_SNORM                   = 81,
	DDSPP_FORMAT_BC5_TYPELESS                = 82,
	DDSPP_FORMAT_BC5_UNORM                   = 83,
	DDSPP_FORMAT_BC5_SNORM                   = 84,
	DDSPP_FORMAT_B5G6R5_UNORM                = 85,
	DDSPP_FORMAT_B5G5R5A1_UNORM              = 86,
	DDSPP_FORMAT_B8G8R8A8_UNORM              = 87,
	DDSPP_FORMAT_B8G8R8X8_UNORM              = 88,
	DDSPP_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
	DDSPP_FORMAT_B8G8R8A8_TYPELESS           = 90,
	DDSPP_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	DDSPP_FORMAT_B8G8R8X8_TYPELESS           = 92,
	DDSPP_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	DDSPP_FORMAT_BC6H_TYPELESS               = 94,
	DDSPP_FORMAT_BC6H_UF16                   = 95,
	DDSPP_FORMAT_BC6H_SF16                   = 96,
	DDSPP_FORMAT_BC7_TYPELESS                = 97,
	DDSPP_FORMAT_BC7_UNORM                   = 98,
	DDSPP_FORMAT_BC7_UNORM_SRGB              = 99,
	DDSPP_FORMAT_AYUV                        = 100,
	DDSPP_FORMAT_Y410                        = 101,
	DDSPP_FORMAT_Y416                        = 102,
	DDSPP_FORMAT_NV12                        = 103,
	DDSPP_FORMAT_P010                        = 104,
	DDSPP_FORMAT_P016                        = 105,
	DDSPP_FORMAT_OPAQUE_420                  = 106,
	DDSPP_FORMAT_YUY2                        = 107,
	DDSPP_FORMAT_Y210                        = 108,
	DDSPP_FORMAT_Y216                        = 109,
	DDSPP_FORMAT_NV11                        = 110,
	DDSPP_FORMAT_AI44                        = 111,
	DDSPP_FORMAT_IA44                        = 112,
	DDSPP_FORMAT_P8                          = 113,
	DDSPP_FORMAT_A8P8                        = 114,
	DDSPP_FORMAT_B4G4R4A4_UNORM              = 115,

	DDSPP_FORMAT_P208                        = 130,
	DDSPP_FORMAT_V208                        = 131,
	DDSPP_FORMAT_V408                        = 132,
	DDSPP_FORMAT_ASTC_4X4_TYPELESS           = 133,
	DDSPP_FORMAT_ASTC_4X4_UNORM              = 134,
	DDSPP_FORMAT_ASTC_4X4_UNORM_SRGB         = 135,
	DDSPP_FORMAT_ASTC_5X4_TYPELESS           = 137,
	DDSPP_FORMAT_ASTC_5X4_UNORM              = 138,
	DDSPP_FORMAT_ASTC_5X4_UNORM_SRGB         = 139,
	DDSPP_FORMAT_ASTC_5X5_TYPELESS           = 141,
	DDSPP_FORMAT_ASTC_5X5_UNORM              = 142,
	DDSPP_FORMAT_ASTC_5X5_UNORM_SRGB         = 143,

	DDSPP_FORMAT_ASTC_6X5_TYPELESS           = 145,
	DDSPP_FORMAT_ASTC_6X5_UNORM              = 146,
	DDSPP_FORMAT_ASTC_6X5_UNORM_SRGB         = 147,

	DDSPP_FORMAT_ASTC_6X6_TYPELESS           = 149,
	DDSPP_FORMAT_ASTC_6X6_UNORM              = 150,
	DDSPP_FORMAT_ASTC_6X6_UNORM_SRGB         = 151,

	DDSPP_FORMAT_ASTC_8X5_TYPELESS           = 153,
	DDSPP_FORMAT_ASTC_8X5_UNORM              = 154,
	DDSPP_FORMAT_ASTC_8X5_UNORM_SRGB         = 155,

	DDSPP_FORMAT_ASTC_8X6_TYPELESS           = 157,
	DDSPP_FORMAT_ASTC_8X6_UNORM              = 158,
	DDSPP_FORMAT_ASTC_8X6_UNORM_SRGB         = 159,

	DDSPP_FORMAT_ASTC_8X8_TYPELESS           = 161,
	DDSPP_FORMAT_ASTC_8X8_UNORM              = 162,
	DDSPP_FORMAT_ASTC_8X8_UNORM_SRGB         = 163,

	DDSPP_FORMAT_ASTC_10X5_TYPELESS          = 165,
	DDSPP_FORMAT_ASTC_10X5_UNORM             = 166,
	DDSPP_FORMAT_ASTC_10X5_UNORM_SRGB        = 167,

	DDSPP_FORMAT_ASTC_10X6_TYPELESS          = 169,
	DDSPP_FORMAT_ASTC_10X6_UNORM             = 170,
	DDSPP_FORMAT_ASTC_10X6_UNORM_SRGB        = 171,

	DDSPP_FORMAT_ASTC_10X8_TYPELESS          = 173,
	DDSPP_FORMAT_ASTC_10X8_UNORM             = 174,
	DDSPP_FORMAT_ASTC_10X8_UNORM_SRGB        = 175,

	DDSPP_FORMAT_ASTC_10X10_TYPELESS         = 177,
	DDSPP_FORMAT_ASTC_10X10_UNORM            = 178,
	DDSPP_FORMAT_ASTC_10X10_UNORM_SRGB       = 179,

	DDSPP_FORMAT_ASTC_12X10_TYPELESS         = 181,
	DDSPP_FORMAT_ASTC_12X10_UNORM            = 182,
	DDSPP_FORMAT_ASTC_12X10_UNORM_SRGB       = 183,

	DDSPP_FORMAT_ASTC_12X12_TYPELESS         = 185,
	DDSPP_FORMAT_ASTC_12X12_UNORM            = 186,
	DDSPP_FORMAT_ASTC_12X12_UNORM_SRGB       = 187,

	DDSPP_FORMAT_FORCE_UINT                  = 0xffffffff
} DDSPP_DXGIFormat;

typedef struct {
	unsigned int size;
	unsigned int flags;
	unsigned int height;
	unsigned int width;
	unsigned int pitchOrLinearSize;
	unsigned int depth;
	unsigned int mipMapCount;
	unsigned int reserved1[11];
	DDSPP_PixelFormat ddspf;
	unsigned int caps;
	unsigned int caps2;
	unsigned int caps3;
	unsigned int caps4;
	unsigned int reserved2;
} DDSPP_Header;

// static_assert(sizeof(DDSPP_Header) == 124, "DDS DDSPP_Header size mismatch");

typedef struct {
	DDSPP_DXGIFormat dxgiFormat;
	DDSPP_DXGIResourceDimension resourceDimension;
	unsigned int miscFlag;
	unsigned int arraySize;
	unsigned int reserved;
} DDSPP_HeaderDXT10;

// static_assert(sizeof(DDSPP_HeaderDXT10) == 20, "DDS DX10 Extended DDSPP_Header size mismatch");

// Maximum possible size of header. Use this to read in only the header, decode, seek to the real header size, then read in the rest of the image data
static const int DDSPP_MAX_HEADER_SIZE = sizeof(DDSPP_DDS_MAGIC) + sizeof(DDSPP_Header) + sizeof(DDSPP_HeaderDXT10);

typedef enum {
	DDSPP_Success,
	DDSPP_Error
} DDSPP_Result;

typedef enum {
	Texture1D,
	Texture2D,
	Texture3D,
	Cubemap,
} DDSPP_TextureType;

typedef struct {
	DDSPP_DXGIFormat format;
	DDSPP_TextureType type;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	unsigned int numMips;
	unsigned int arraySize;
	unsigned int rowPitch; // Row pitch for mip 0
	unsigned int depthPitch; // Size of mip 0
	unsigned int bitsPerPixelOrBlock; // If compressed bits per block, else bits per pixel
	unsigned int blockWidth; // Width of block in pixels (1 if uncompressed)
	unsigned int blockHeight;// Height of block in pixels (1 if uncompressed)
	bool compressed;
	bool srgb;
	unsigned int headerSize; // Actual size of header, use this to get to image data
} DDSPP_Descriptor;

static inline const bool DDSPP_IsDXT10(const DDSPP_Header* header)
{
	const DDSPP_PixelFormat* ddspf = &header->ddspf;
	return (ddspf->flags & DDSPP_DDS_FOURCC) && (ddspf->fourCC == DDSPP_FOURCC_DXT10);
}

static inline const bool DDSPP_IsCompressed(DDSPP_DXGIFormat format)
{
	return (format >= DDSPP_FORMAT_BC1_UNORM && format <= DDSPP_FORMAT_BC5_SNORM) || 
		(format >= DDSPP_FORMAT_BC6H_TYPELESS && format <= DDSPP_FORMAT_BC7_UNORM_SRGB) || 
		(format >= DDSPP_FORMAT_ASTC_4X4_TYPELESS && format <= DDSPP_FORMAT_ASTC_12X12_UNORM_SRGB);
}

static inline const bool DDSPP_IsSRGB(DDSPP_DXGIFormat format)
{
	switch(format)
	{
	case DDSPP_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DDSPP_FORMAT_BC1_UNORM_SRGB:
	case DDSPP_FORMAT_BC2_UNORM_SRGB:
	case DDSPP_FORMAT_BC3_UNORM_SRGB:
	case DDSPP_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DDSPP_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DDSPP_FORMAT_BC7_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_4X4_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_5X4_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_5X5_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_6X5_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_6X6_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_8X5_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_8X6_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_8X8_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_10X5_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_10X6_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_10X8_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_10X10_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_12X10_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_12X12_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

static inline unsigned int DDSPP_GetBitsPerPixelOrBlock(DDSPP_DXGIFormat format)
{
	if(format >= DDSPP_FORMAT_ASTC_4X4_TYPELESS && format <= DDSPP_FORMAT_ASTC_12X12_UNORM_SRGB)
	{
		return 128; // All ASTC blocks are the same size
	}

	switch(format)
	{
	case DDSPP_FORMAT_R1_UNORM:
		return 1;
	case DDSPP_FORMAT_R8_TYPELESS:
	case DDSPP_FORMAT_R8_UNORM:
	case DDSPP_FORMAT_R8_UINT:
	case DDSPP_FORMAT_R8_SNORM:
	case DDSPP_FORMAT_R8_SINT:
	case DDSPP_FORMAT_A8_UNORM:
	case DDSPP_FORMAT_AI44:
	case DDSPP_FORMAT_IA44:
	case DDSPP_FORMAT_P8:
		return 8;
	case DDSPP_FORMAT_NV12:
	case DDSPP_FORMAT_OPAQUE_420:
	case DDSPP_FORMAT_NV11:
		return 12;
	case DDSPP_FORMAT_R8G8_TYPELESS:
	case DDSPP_FORMAT_R8G8_UNORM:
	case DDSPP_FORMAT_R8G8_UINT:
	case DDSPP_FORMAT_R8G8_SNORM:
	case DDSPP_FORMAT_R8G8_SINT:
	case DDSPP_FORMAT_R16_TYPELESS:
	case DDSPP_FORMAT_R16_FLOAT:
	case DDSPP_FORMAT_D16_UNORM:
	case DDSPP_FORMAT_R16_UNORM:
	case DDSPP_FORMAT_R16_UINT:
	case DDSPP_FORMAT_R16_SNORM:
	case DDSPP_FORMAT_R16_SINT:
	case DDSPP_FORMAT_B5G6R5_UNORM:
	case DDSPP_FORMAT_B5G5R5A1_UNORM:
	case DDSPP_FORMAT_A8P8:
	case DDSPP_FORMAT_B4G4R4A4_UNORM:
		return 16;
	case DDSPP_FORMAT_P010:
	case DDSPP_FORMAT_P016:
		return 24;
	case DDSPP_FORMAT_BC1_UNORM:
	case DDSPP_FORMAT_BC1_UNORM_SRGB:
	case DDSPP_FORMAT_BC1_TYPELESS:
	case DDSPP_FORMAT_BC4_UNORM:
	case DDSPP_FORMAT_BC4_SNORM:
	case DDSPP_FORMAT_BC4_TYPELESS:
	case DDSPP_FORMAT_R16G16B16A16_TYPELESS:
	case DDSPP_FORMAT_R16G16B16A16_FLOAT:
	case DDSPP_FORMAT_R16G16B16A16_UNORM:
	case DDSPP_FORMAT_R16G16B16A16_UINT:
	case DDSPP_FORMAT_R16G16B16A16_SNORM:
	case DDSPP_FORMAT_R16G16B16A16_SINT:
	case DDSPP_FORMAT_R32G32_TYPELESS:
	case DDSPP_FORMAT_R32G32_FLOAT:
	case DDSPP_FORMAT_R32G32_UINT:
	case DDSPP_FORMAT_R32G32_SINT:
	case DDSPP_FORMAT_R32G8X24_TYPELESS:
	case DDSPP_FORMAT_D32_FLOAT_S8X24_UINT:
	case DDSPP_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DDSPP_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DDSPP_FORMAT_Y416:
	case DDSPP_FORMAT_Y210:
	case DDSPP_FORMAT_Y216:
		return 64;
	case DDSPP_FORMAT_R32G32B32_TYPELESS:
	case DDSPP_FORMAT_R32G32B32_FLOAT:
	case DDSPP_FORMAT_R32G32B32_UINT:
	case DDSPP_FORMAT_R32G32B32_SINT:
		return 96;
	case DDSPP_FORMAT_BC2_UNORM:
	case DDSPP_FORMAT_BC2_UNORM_SRGB:
	case DDSPP_FORMAT_BC2_TYPELESS:
	case DDSPP_FORMAT_BC3_UNORM:
	case DDSPP_FORMAT_BC3_UNORM_SRGB:
	case DDSPP_FORMAT_BC3_TYPELESS:
	case DDSPP_FORMAT_BC5_UNORM:
	case DDSPP_FORMAT_BC5_SNORM:
	case DDSPP_FORMAT_BC6H_UF16:
	case DDSPP_FORMAT_BC6H_SF16:
	case DDSPP_FORMAT_BC7_UNORM:
	case DDSPP_FORMAT_BC7_UNORM_SRGB:
	case DDSPP_FORMAT_R32G32B32A32_TYPELESS:
	case DDSPP_FORMAT_R32G32B32A32_FLOAT:
	case DDSPP_FORMAT_R32G32B32A32_UINT:
	case DDSPP_FORMAT_R32G32B32A32_SINT:
		return 128;
	default:
		return 32; // Most formats are 32 bits per pixel
		break;
	}
}

static inline const void DDSPP_GetBlockSize(DDSPP_DXGIFormat format, unsigned int* blockWidth, unsigned int* blockHeight)
{
	switch(format)
	{
	case DDSPP_FORMAT_BC1_UNORM:
	case DDSPP_FORMAT_BC1_UNORM_SRGB:
	case DDSPP_FORMAT_BC1_TYPELESS:
	case DDSPP_FORMAT_BC4_UNORM:
	case DDSPP_FORMAT_BC4_SNORM:
	case DDSPP_FORMAT_BC4_TYPELESS:
	case DDSPP_FORMAT_BC2_UNORM:
	case DDSPP_FORMAT_BC2_UNORM_SRGB:
	case DDSPP_FORMAT_BC2_TYPELESS:
	case DDSPP_FORMAT_BC3_UNORM:
	case DDSPP_FORMAT_BC3_UNORM_SRGB:
	case DDSPP_FORMAT_BC3_TYPELESS:
	case DDSPP_FORMAT_BC5_UNORM:
	case DDSPP_FORMAT_BC5_SNORM:
	case DDSPP_FORMAT_BC6H_UF16:
	case DDSPP_FORMAT_BC6H_SF16:
	case DDSPP_FORMAT_BC7_UNORM:
	case DDSPP_FORMAT_BC7_UNORM_SRGB:
	case DDSPP_FORMAT_ASTC_4X4_TYPELESS:
	case DDSPP_FORMAT_ASTC_4X4_UNORM:
	case DDSPP_FORMAT_ASTC_4X4_UNORM_SRGB:
		*blockWidth = 4; *blockHeight = 4;
		break;
	case DDSPP_FORMAT_ASTC_5X4_TYPELESS:
	case DDSPP_FORMAT_ASTC_5X4_UNORM:
	case DDSPP_FORMAT_ASTC_5X4_UNORM_SRGB:
		*blockWidth = 5; *blockHeight = 4;
		break;
	case DDSPP_FORMAT_ASTC_5X5_TYPELESS:
	case DDSPP_FORMAT_ASTC_5X5_UNORM:
	case DDSPP_FORMAT_ASTC_5X5_UNORM_SRGB:
		*blockWidth = 5; *blockHeight = 5;
		break;
	case DDSPP_FORMAT_ASTC_6X5_TYPELESS:
	case DDSPP_FORMAT_ASTC_6X5_UNORM:
	case DDSPP_FORMAT_ASTC_6X5_UNORM_SRGB:
		*blockWidth = 6; *blockHeight = 5;
	case DDSPP_FORMAT_ASTC_6X6_TYPELESS:
	case DDSPP_FORMAT_ASTC_6X6_UNORM:
	case DDSPP_FORMAT_ASTC_6X6_UNORM_SRGB:
		*blockWidth = 6; *blockHeight = 6;
	case DDSPP_FORMAT_ASTC_8X5_TYPELESS:
	case DDSPP_FORMAT_ASTC_8X5_UNORM:
	case DDSPP_FORMAT_ASTC_8X5_UNORM_SRGB:
		*blockWidth = 8; *blockHeight = 5;
	case DDSPP_FORMAT_ASTC_8X6_TYPELESS:
	case DDSPP_FORMAT_ASTC_8X6_UNORM:
	case DDSPP_FORMAT_ASTC_8X6_UNORM_SRGB:
		*blockWidth = 8; *blockHeight = 6;
	case DDSPP_FORMAT_ASTC_8X8_TYPELESS:
	case DDSPP_FORMAT_ASTC_8X8_UNORM:
	case DDSPP_FORMAT_ASTC_8X8_UNORM_SRGB:
		*blockWidth = 8; *blockHeight = 8;
	case DDSPP_FORMAT_ASTC_10X5_TYPELESS:
	case DDSPP_FORMAT_ASTC_10X5_UNORM:
	case DDSPP_FORMAT_ASTC_10X5_UNORM_SRGB:
		*blockWidth = 10; *blockHeight = 5;
	case DDSPP_FORMAT_ASTC_10X6_TYPELESS:
	case DDSPP_FORMAT_ASTC_10X6_UNORM:
	case DDSPP_FORMAT_ASTC_10X6_UNORM_SRGB:
		*blockWidth = 10; *blockHeight = 6;
	case DDSPP_FORMAT_ASTC_10X8_TYPELESS:
	case DDSPP_FORMAT_ASTC_10X8_UNORM:
	case DDSPP_FORMAT_ASTC_10X8_UNORM_SRGB:
		*blockWidth = 10; *blockHeight = 8;
	case DDSPP_FORMAT_ASTC_10X10_TYPELESS:
	case DDSPP_FORMAT_ASTC_10X10_UNORM:
	case DDSPP_FORMAT_ASTC_10X10_UNORM_SRGB:
		*blockWidth = 10; *blockHeight = 10;
	case DDSPP_FORMAT_ASTC_12X10_TYPELESS:
	case DDSPP_FORMAT_ASTC_12X10_UNORM:
	case DDSPP_FORMAT_ASTC_12X10_UNORM_SRGB:
		*blockWidth = 12; *blockHeight = 10;
	case DDSPP_FORMAT_ASTC_12X12_TYPELESS:
	case DDSPP_FORMAT_ASTC_12X12_UNORM:
	case DDSPP_FORMAT_ASTC_12X12_UNORM_SRGB:
		*blockWidth = 12; *blockHeight = 12;
	default:
		*blockWidth = 1; *blockHeight = 1;
		break;
	}

	return;
}

static inline unsigned int DDSPP_GetRowPitchEx(unsigned int width, unsigned int bitsPerPixelOrBlock, unsigned int blockWidth, unsigned int mip)
{
	// Shift width by mipmap index, round to next block size and round to next byte (for the rare less than 1 byte per pixel formats)
	// E.g. width = 119, mip = 3, BC1 compression
	// ((((119 >> 2) + 4 - 1) / 4) * 64) / 8 = 64 bytes
	return ((((width >> mip) + blockWidth - 1) / blockWidth) * bitsPerPixelOrBlock + 7) / 8;
}

// Returns number of bytes for each row of a given mip. Valid range is [0, desc.numMips)
static inline unsigned int DDSPP_GetRowPitch(const DDSPP_Descriptor* desc, unsigned int mip)
{
	return DDSPP_GetRowPitchEx(desc->width, desc->bitsPerPixelOrBlock, desc->blockWidth, mip);
}

static inline DDSPP_Result DDSPP_DecodeHeader(const unsigned char* sourceData, DDSPP_Descriptor* desc)
{
	unsigned int magic = *(unsigned int*)(sourceData); // First 4 bytes are the magic DDS number

	if (magic != DDSPP_DDS_MAGIC)
	{
		return DDSPP_Error;
	}

	const DDSPP_Header header = *(const DDSPP_Header*)(sourceData + sizeof(DDSPP_DDS_MAGIC));
	const DDSPP_PixelFormat* ddspf = &header.ddspf;
	bool dxt10Extension = DDSPP_IsDXT10(&header);
	const DDSPP_HeaderDXT10 dxt10Header = *(const DDSPP_HeaderDXT10*)(sourceData + sizeof(DDSPP_DDS_MAGIC) + sizeof(DDSPP_Header));

	// Read basic data from the header
	desc->width = header.width > 0 ? header.width : 1;
	desc->height = header.height > 0 ? header.height : 1;
	desc->depth = header.depth > 0 ? header.depth : 1;
	desc->numMips = header.mipMapCount > 0 ? header.mipMapCount : 1;

	// Set some sensible defaults
	desc->arraySize = 1;
	desc->srgb = false;
	desc->type = Texture2D;
	desc->format = DDSPP_FORMAT_UNKNOWN;

	if (dxt10Extension)
	{
		desc->format = dxt10Header.dxgiFormat;

		desc->arraySize = dxt10Header.arraySize;

		switch(dxt10Header.resourceDimension)
		{
		case DXGI_Texture1D:
			desc->depth = 1;
			desc->type = Texture1D;
			break;
		case DXGI_Texture2D:
			desc->depth = 1;

			if(dxt10Header.miscFlag & DDSPP_DXGI_MISC_FLAG_CUBEMAP)
			{
				desc->type = Cubemap;
			}
			else
			{
				desc->type = Texture2D;
			}

			break;
		case DXGI_Texture3D:
			desc->type = Texture3D;
			desc->arraySize = 1; // There are no 3D texture arrays
			break;
		default:
			break;
		}
	}
	else
	{
		if(ddspf->flags & DDSPP_DDS_FOURCC)
		{
			unsigned int fourCC = ddspf->fourCC;

			switch(fourCC)
			{
				// Compressed
			case DDSPP_FOURCC_DXT1:    desc->format = DDSPP_FORMAT_BC1_UNORM; break;
			case DDSPP_FOURCC_DXT2:    // fallthrough
			case DDSPP_FOURCC_DXT3:    desc->format = DDSPP_FORMAT_BC2_UNORM; break;
			case DDSPP_FOURCC_DXT4:    // fallthrough
			case DDSPP_FOURCC_DXT5:    desc->format = DDSPP_FORMAT_BC3_UNORM; break;
			case DDSPP_FOURCC_ATI1:    // fallthrough
			case DDSPP_FOURCC_BC4U:    desc->format = DDSPP_FORMAT_BC4_UNORM; break;
			case DDSPP_FOURCC_BC4S:    desc->format = DDSPP_FORMAT_BC4_SNORM; break;
			case DDSPP_FOURCC_ATI2:    // fallthrough
			case DDSPP_FOURCC_BC5U:    desc->format = DDSPP_FORMAT_BC5_UNORM; break;
			case DDSPP_FOURCC_BC5S:    desc->format = DDSPP_FORMAT_BC5_SNORM; break;

				// Video
			case DDSPP_FOURCC_RGBG:    desc->format = DDSPP_FORMAT_R8G8_B8G8_UNORM; break;
			case DDSPP_FOURCC_GRBG:    desc->format = DDSPP_FORMAT_G8R8_G8B8_UNORM; break;
			case DDSPP_FOURCC_YUY2:    desc->format = DDSPP_FORMAT_YUY2; break;

				// Packed
			case DDSPP_FOURCC_R5G6B5:  desc->format = DDSPP_FORMAT_B5G6R5_UNORM; break;
			case DDSPP_FOURCC_RGB5A1:  desc->format = DDSPP_FORMAT_B5G5R5A1_UNORM; break;
			case DDSPP_FOURCC_RGBA4:   desc->format = DDSPP_FORMAT_B4G4R4A4_UNORM; break;

				// Uncompressed
			case DDSPP_FOURCC_A8:          desc->format = DDSPP_FORMAT_R8_UNORM; break;
			case DDSPP_FOURCC_A2B10G10R10: desc->format = DDSPP_FORMAT_R10G10B10A2_UNORM; break;
			case DDSPP_FOURCC_RGBA16U:     desc->format = DDSPP_FORMAT_R16G16B16A16_UNORM; break;
			case DDSPP_FOURCC_RGBA16S:     desc->format = DDSPP_FORMAT_R16G16B16A16_SNORM; break;
			case DDSPP_FOURCC_R16F:        desc->format = DDSPP_FORMAT_R16_FLOAT; break;
			case DDSPP_FOURCC_RG16F:       desc->format = DDSPP_FORMAT_R16G16_FLOAT; break;
			case DDSPP_FOURCC_RGBA16F:     desc->format = DDSPP_FORMAT_R16G16B16A16_FLOAT; break;
			case DDSPP_FOURCC_R32F:        desc->format = DDSPP_FORMAT_R32_FLOAT; break;
			case DDSPP_FOURCC_RG32F:       desc->format = DDSPP_FORMAT_R32G32_FLOAT; break;
			case DDSPP_FOURCC_RGBA32F:     desc->format = DDSPP_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
		else if(ddspf->flags & DDSPP_DDS_RGB)
		{
			switch(ddspf->RGBBitCount)
			{
			case 32:
				if(DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
				{
					desc->format = DDSPP_FORMAT_R8G8B8A8_UNORM;
				}
				else if(DDSPP_IsRGBAMask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
				{
					desc->format = DDSPP_FORMAT_B8G8R8A8_UNORM;
				}
				else if(DDSPP_IsRGBAMask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_B8G8R8X8_UNORM;
				}

				// No DXGI format maps to (0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8
				// No DXGI format maps to (0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10

				// Note that many common DDS reader/writers (including D3DX) swap the
				// the RED/BLUE masks for 10:10:10:2 formats. We assume
				// below that the 'backwards' header mask is being used since it is most
				// likely written by D3DX. The more robust solution is to use the 'DX10'
				// header extension and specify the DXGI_FORMAT_DDSPP_FORMAT_R10G10B10A2_UNORM format directly

				// For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data
				else if (DDSPP_IsRGBAMask(ddspf, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
				{
					desc->format = DDSPP_FORMAT_R10G10B10A2_UNORM;
				}
				else if (DDSPP_IsRGBAMask(ddspf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_R16G16_UNORM;
				}
				else if (DDSPP_IsRGBAMask(ddspf, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
				{
					// The only 32-bit color channel format in D3D9 was R32F
					desc->format = DDSPP_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
				}
				break;
			case 24:
				if(DDSPP_IsRGBMask(ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff))
				{
					desc->format = DDSPP_FORMAT_B8G8R8X8_UNORM;
				}
				break;
			case 16:
				if (DDSPP_IsRGBAMask(ddspf, 0x7c00, 0x03e0, 0x001f, 0x8000))
				{
					desc->format = DDSPP_FORMAT_B5G5R5A1_UNORM;
				}
				else if (DDSPP_IsRGBAMask(ddspf, 0xf800, 0x07e0, 0x001f, 0x0000))
				{
					desc->format = DDSPP_FORMAT_B5G6R5_UNORM;
				}
				else if (DDSPP_IsRGBAMask(ddspf, 0x0f00, 0x00f0, 0x000f, 0xf000))
				{
					desc->format = DDSPP_FORMAT_B4G4R4A4_UNORM;
				}
				// No DXGI format maps to (0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5.
				// No DXGI format maps to (0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4.
				// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
				break;
			default:
				break;
			}
		}
		else if (ddspf->flags & DDSPP_DDS_LUMINANCE)
		{
			switch(ddspf->RGBBitCount)
			{
			case 16:
				if (DDSPP_IsRGBAMask(ddspf, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension.
				}
				if (DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
				{
					desc->format = DDSPP_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension.
				}
				break;
			case 8:
				if (DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to (0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4

				if (DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
				{
					desc->format = DDSPP_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
				}
				break;
			}
		}
		else if (ddspf->flags & DDSPP_DDS_ALPHA)
		{
			if (ddspf->RGBBitCount == 8)
			{
				desc->format = DDSPP_FORMAT_A8_UNORM;
			}
		}
		else if (ddspf->flags & DDSPP_DDS_BUMPDUDV)
		{
			if (ddspf->RGBBitCount == 32)
			{
				if (DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
				{
					desc->format = DDSPP_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (DDSPP_IsRGBAMask(ddspf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to (0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
			}
			else if (ddspf->RGBBitCount == 16)
			{
				if (DDSPP_IsRGBAMask(ddspf, 0x000000ff, 0x0000ff00, 0x00000000, 0x00000000))
				{
					desc->format = DDSPP_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
				}
			}
		}

		if((header.flags & DDSPP_DDS_HEADER_FLAGS_VOLUME) || (header.caps2 & DDSPP_DDS_HEADER_CAPS2_VOLUME))
		{
			desc->type = Texture3D;
		}
		else if (header.caps2 & DDSPP_DDS_HEADER_CAPS2_CUBEMAP)
		{
			if((header.caps2 & DDSPP_DDS_HEADER_CAPS2_CUBEMAP_ALLFACES) != DDSPP_DDS_HEADER_CAPS2_CUBEMAP_ALLFACES)
			{
				return DDSPP_Error;
			}

			desc->type = Cubemap;
			desc->arraySize = 1;
			desc->depth = 1;
		}
		else
		{
			desc->type = Texture2D;
		}
	}

	desc->compressed = DDSPP_IsCompressed(desc->format);
	desc->srgb = DDSPP_IsSRGB(desc->format);
	desc->bitsPerPixelOrBlock = DDSPP_GetBitsPerPixelOrBlock(desc->format);
	DDSPP_GetBlockSize(desc->format, &desc->blockWidth, &desc->blockHeight);

	desc->rowPitch = DDSPP_GetRowPitchEx(desc->width, desc->bitsPerPixelOrBlock, desc->blockWidth, 0);
	desc->depthPitch = desc->rowPitch * desc->height / desc->blockHeight;
	desc->headerSize = sizeof(DDSPP_DDS_MAGIC) + sizeof(DDSPP_Header) + (dxt10Extension ? sizeof(DDSPP_HeaderDXT10) : 0);

	return DDSPP_Success;
}

static inline void DDSPP_EncodeHeader(const DDSPP_DXGIFormat format, const unsigned int width, const unsigned int height, const unsigned int depth,
	const DDSPP_TextureType type, const unsigned int mipCount, const unsigned int arraySize, 
	DDSPP_Header* header, DDSPP_HeaderDXT10* dxt10Header)
{
	header->size = sizeof(DDSPP_Header);

	// Fill in header flags
	header->flags = DDSPP_DDS_HEADER_FLAGS_CAPS | DDSPP_DDS_HEADER_FLAGS_HEIGHT | DDSPP_DDS_HEADER_FLAGS_WIDTH | DDSPP_DDS_HEADER_FLAGS_PIXELFORMAT;
	header->caps = DDSPP_DDS_HEADER_CAPS_TEXTURE;
	header->caps2 = 0;

	if (mipCount > 1)
	{
		header->flags |= DDSPP_DDS_HEADER_FLAGS_MIPMAP;
		header->caps |= DDSPP_DDS_HEADER_CAPS_COMPLEX | DDSPP_DDS_HEADER_CAPS_MIPMAP;
	}

	unsigned int bitsPerPixelOrBlock = DDSPP_GetBitsPerPixelOrBlock(format);

	if (DDSPP_IsCompressed(format))
	{
		header->flags |= DDSPP_DDS_HEADER_FLAGS_LINEARSIZE;
		unsigned int blockWidth, blockHeight;
		DDSPP_GetBlockSize(format, &blockWidth, &blockHeight);
		header->pitchOrLinearSize = width * height * bitsPerPixelOrBlock / (8 * blockWidth * blockHeight);
	}
	else
	{
		header->flags |= DDSPP_DDS_HEADER_FLAGS_PITCH;
		header->pitchOrLinearSize = width * bitsPerPixelOrBlock / 8;
	}

	header->height = height;
	header->width = width;
	header->depth = depth;
	header->mipMapCount = mipCount;
	header->reserved1[0] = header->reserved1[1] = header->reserved1[2] = 0;
	header->reserved1[3] = header->reserved1[4] = header->reserved1[5] = 0;
	header->reserved1[6] = header->reserved1[7] = header->reserved1[8] = 0;
	header->reserved1[9] = header->reserved1[10] = 0;

	// Fill in pixel format
	DDSPP_PixelFormat* ddspf = &header->ddspf;
	ddspf->size = sizeof(DDSPP_PixelFormat);
	ddspf->fourCC = DDSPP_FOURCC_DXT10;
	ddspf->flags = 0;
	ddspf->flags |= DDSPP_DDS_FOURCC;

	dxt10Header->dxgiFormat = format;
	dxt10Header->arraySize = arraySize;
	dxt10Header->miscFlag = 0;

	if (type == Texture1D)
	{
		dxt10Header->resourceDimension = DXGI_Texture1D;
	}
	else if (type == Texture2D)
	{
		dxt10Header->resourceDimension = DXGI_Texture2D;
	}
	else if (type == Cubemap)
	{
		dxt10Header->resourceDimension = DXGI_Texture2D;
		dxt10Header->miscFlag |= DDSPP_DXGI_MISC_FLAG_CUBEMAP;
		header->caps |= DDSPP_DDS_HEADER_CAPS_COMPLEX;
		header->caps2 |= DDSPP_DDS_HEADER_CAPS2_CUBEMAP | DDSPP_DDS_HEADER_CAPS2_CUBEMAP_ALLFACES;
	}
	else if (type == Texture3D)
	{
		dxt10Header->resourceDimension = DXGI_Texture3D;
		header->flags |= DDSPP_DDS_HEADER_FLAGS_VOLUME;
		header->caps2 |= DDSPP_DDS_HEADER_CAPS2_VOLUME;
	}

	// dxt10Header->miscFlag TODO Alpha Mode

	// Unused
	header->caps3 = 0;
	header->caps4 = 0;
	header->reserved2 = 0;
}

// Returns the offset from the base pointer to the desired mip and slice
// Slice is either a texture from an array, a face from a cubemap, or a 2D slice of a volume texture
static inline unsigned int DDSPP_GetOffset(const DDSPP_Descriptor* desc, const unsigned int mip, const unsigned int slice)
{
	// The mip/slice arrangement is different between texture arrays and volume textures
	//
	// Arrays
	//  __________  _____  __  __________  _____  __  __________  _____  __ 
	// |          ||     ||__||          ||     ||__||          ||     ||__|
	// |          ||_____|    |          ||_____|    |          ||_____|
	// |          |           |          |           |          |
	// |__________|           |__________|           |__________|
	//
	// Volume
	//  __________  __________  __________  _____  _____  _____  __  __  __ 
	// |          ||          ||          ||     ||     ||     ||__||__||__|
	// |          ||          ||          ||_____||_____||_____|
	// |          ||          ||          |
	// |__________||__________||__________|
	//

	unsigned long long offset = 0;
	unsigned long long mip0Size = desc->depthPitch * 8; // Work in bits

	if (desc->type == Texture3D)
	{
		for (unsigned int m = 0; m < mip; ++m)
		{
			unsigned long long mipSize = mip0Size >> 2 * m;
			offset += mipSize * desc->numMips;
		}

		unsigned long long lastMip = mip0Size >> 2 * mip;

		offset += lastMip * slice;
	}
	else
	{
		unsigned long long mipChainSize = 0;

		for (unsigned int m = 0; m < desc->numMips; ++m)
		{
			unsigned long long mipSize = mip0Size >> 2 * m; // Divide by 2 in width and height
			mipChainSize += mipSize > desc->bitsPerPixelOrBlock ? mipSize : desc->bitsPerPixelOrBlock;
		}

		offset += mipChainSize * slice;

		for (unsigned int m = 0; m < mip; ++m)
		{
			unsigned long long mipSize = mip0Size >> 2 * m; // Divide by 2 in width and height
			offset += mipSize > desc->bitsPerPixelOrBlock ? mipSize : desc->bitsPerPixelOrBlock;
		}
	}

	offset /= 8; // Back to bytes

	return (unsigned int)offset;
}