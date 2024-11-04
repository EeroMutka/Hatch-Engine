// This file is part of the Hatch codebase, written by Eero Mutka.
// For educational purposes only.

struct PSInput {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(in uint vert_id : SV_VertexID)
{
	float2 uv = float2((vert_id << 1) & 2, vert_id & 2);
	float4 pos = float4(uv * 2.0 - float2(1.0, 1.0), 0.0, 1.0);
	uv.y = 1. - uv.y;
	PSInput output;
	output.pos = pos;
	output.uv = uv;
	return output;
}

// ----------------------------------------------------------------------------------

SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(texture0.Sample(sampler0, input.uv).xyz, 1.0);
	// return float4(input.uv, 0, 1);
}
