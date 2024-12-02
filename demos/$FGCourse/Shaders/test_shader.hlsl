#pragma pack_matrix(row_major)

cbuffer constants : register(b0) {
    float4x4 ws_to_cs;
}

// Texture2D    mytexture : register(t0);
// SamplerState mysampler : register(s0);

struct VertexInput {
	float3 position : POS;
	// float2 uv       : UVPOS;
	// float4 color    : COL;
};

struct PixelInput {
	float4 position : SV_POSITION;
	float3 position_ws : TEXCOORD0;
	// float4 color    : COL;
};

PixelInput vertex_shader(VertexInput vertex) {
    PixelInput output;
    output.position = mul(float4(vertex.position, 1.0), ws_to_cs);
    
    // apparently, clip space is +Y up by default in D3D11 when rendering to the swapchain. We want it to be +Y down
    output.position.y = -output.position.y;
    
    output.position_ws = vertex.position;
	return output;
}

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    return float4(pixel.position_ws, 1);
}