// This file is part of the Hatch codebase, written by Eero Mutka.
// For educational purposes only.

cbuffer Vertex32BitConstants : register(b0)
{
	float4x4 projection_matrix;
};

struct PSInput {
	float4 pos : SV_POSITION;
	float3 ws_pos : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float2 uv : TEXCOORD2;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : UV)
{
	PSInput output;
	output.pos = mul(projection_matrix, float4(position, 1.0));
	output.ws_pos = position;
	output.normal = normal;
	
	uv.y = 1.-uv.y;
	output.uv = uv;
	return output;
}

SamplerState sampler0 : register(s0);

Texture2D tex_base_color : register(t0);
Texture2D tex_normal : register(t1);
Texture2D tex_occlusion_roughness_metallic : register(t2);

// ----------------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
	// return float4(normalize(input.normal)*0.5 + 0.5, 1.);
	// return float4(frac(input.uv), 0, 1.);
	return tex_base_color.Sample(sampler0, input.uv);
}
