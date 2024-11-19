
struct PSInput {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(in uint vert_id : SV_VertexID)
{
	float2 uv = float2((vert_id << 1) & 2, vert_id & 2);
	PSInput output;
	output.pos = float4(uv * 2.0 - float2(1.0, 1.0), 0.0, 1.0);
	output.uv = uv;
	return output;
}

// ----------------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(frac(input.uv*5.0), 0.0, 1.0);
}
