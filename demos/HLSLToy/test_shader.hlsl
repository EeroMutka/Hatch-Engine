
struct PSInput {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(in uint vert_id : SV_VertexID)
{
	PSInput output;
	output.uv = float2((vert_id << 1) & 2, vert_id & 2) * 2.0 - float2(1.0, 1.0);
	output.pos = float4(output.uv.x, output.uv.y, 0, 1);
	return output;
}

// ----------------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(input.uv.x, input.uv.y, 0, 0.5);
}
