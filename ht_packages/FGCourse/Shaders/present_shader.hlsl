#pragma pack_matrix(row_major)

struct PixelInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PixelInput vertex_shader(uint vertex_id : SV_VertexID) {
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    float2 pos = uv * 2.0 - 1.0;
    uv.y = 1. - uv.y;
    
    PixelInput output;
    output.position = float4(pos.x, pos.y, 0., 1.);
    output.uv = uv;
    return output;
}

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    float3 col = mytexture.Sample(mysampler, pixel.uv).rgb;
    return float4(col, 1);
}
