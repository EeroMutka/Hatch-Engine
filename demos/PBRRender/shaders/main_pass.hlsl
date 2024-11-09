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

// TODO: pass tangent-space as a vertex input and compare the performance
// against generating the tangent space on the fly in the pixel shader
float3x3 DeriveTangentSpaceFromPSDerivatives(float2 uv, float3 position, float3 normal) {
	float2 dx_uv = ddx(uv);
	float2 dy_uv = ddy(uv);
	float3 dx_position = ddx(position);
	float3 dy_position = ddy(position);
	
	float3x3 TBN; // tangent-binormal-normal matrix
	if (dx_uv.x*dy_uv.y - dx_uv.y*dy_uv.x < 0) {
		// Derive tangent using texcoord-X
		float3 denormB = (dx_position*dy_uv.x - dy_position*dx_uv.x);
		float3 B = normalize(denormB - normal*dot(normal, denormB)); // Tangent-space +Y direction
		float3 T = cross(B, normal);
		TBN = float3x3(T, B, normal);
	}
	else {
		// Derive bitangent using texcoord-Y
		float3 denormT = dx_position*dy_uv.y - dy_position*dx_uv.y;
		float3 T = normalize(denormT - normal*dot(normal, denormT)); // Tangent-space +X direction
		float3 B = cross(T, normal);
		TBN = float3x3(T, B, normal);
	}
	return TBN;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 normal = normalize(input.normal);
	// return float4(normalize(input.normal)*0.5 + 0.5, 1.);
	// return float4(frac(input.uv), 0, 1.);
	
	// normal in tangent-space
	float3 n_ts = tex_normal.Sample(sampler0, input.uv).xyz * 2.0 - 1.0;
	n_ts.z = sqrt(1.0 - n_ts.x*n_ts.x - n_ts.y*n_ts.y); // x^2 + y^2 + z^2 = 1
	
	float3x3 TBN = DeriveTangentSpaceFromPSDerivatives(input.uv, input.ws_pos, normal);
	float3 normal_adjusted = mul(n_ts, TBN);
	
	float lightness = dot(normal_adjusted, normalize(float3(0.5, 0.9, 1)))*0.5 + 0.5;
	return tex_base_color.Sample(sampler0, input.uv) * lightness;
	// return float4(normal_adjusted, 1);
}
