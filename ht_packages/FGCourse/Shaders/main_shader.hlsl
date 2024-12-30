#pragma pack_matrix(row_major)

cbuffer constants : register(b0) {
    float4x4 ws_to_cs;
}

struct VertexInput {
	float3 position : POS;
	float2 uv : UV;
	float3 normal : NORMAL;
};

struct PixelInput {
	float4 position : SV_POSITION;
	float3 position_ws : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

PixelInput vertex_shader(VertexInput vertex) {
    PixelInput output;
    output.position = mul(float4(vertex.position, 1.0), ws_to_cs);
    
    // apparently, clip space is +Y up by default in D3D11 when rendering to the swapchain. We want it to be +Y down
    output.position.y = -output.position.y;
    
    output.position_ws = vertex.position;
    output.uv = vertex.uv;
    output.normal = vertex.normal;
	return output;
}

Texture2D mytexture : register(t0);
SamplerState mysampler : register(s0);

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    float3 normal = normalize(pixel.normal);
    float lightness = max(dot(normal, float3(0, 0, 1)), 0.3);
    
    float3 col = mytexture.Sample(mysampler, pixel.uv * 5).rgb;

    //float3 normal = normalize(cross(ddx(pixel.position_ws), ddy(pixel.position_ws)));
    return float4(col * lightness, 1);
    //return float4(normal*0.5  + 0.5, 1);
    //return float4(pixel.position_ws, 1);
}