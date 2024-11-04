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
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL)
{
	PSInput output;
	output.pos = mul(projection_matrix, float4(position, 1.0));
	output.ws_pos = position;
	output.normal = normal;
	return output;
}

// ----------------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(normalize(input.normal)*0.5 + 0.5, 1.);
	// return float4(frac(input.ws_pos*10), 1.);
}
