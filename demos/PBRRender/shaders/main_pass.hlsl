
struct PSInput {
	float4 pos : SV_POSITION;
};

PSInput VSMain(float2 position : POSITION)
{
	PSInput output;
	output.pos = float4(position, 0.0, 1.0);
	return output;
}

// ----------------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(1., 0., 0., 1.);
}
