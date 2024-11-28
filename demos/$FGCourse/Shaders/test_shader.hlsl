// cbuffer constants : register(b0) {
// 	float2 pixel_size;
// }

// Texture2D    mytexture : register(t0);
// SamplerState mysampler : register(s0);

struct VertexInput {
	float3 position : POS;
	// float2 uv       : UVPOS;
	// float4 color    : COL;
};

struct PixelInput {
	float4 position : SV_POSITION;
	// float2 uv       : UVPOS;
	// float4 color    : COL;
};

PixelInput vertex_shader(VertexInput vertex) {
	PixelInput output;
	output.position = float4(vertex.position, 1.0);
	return output;
}

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
	return float4(1, 0, 0, 1);
}