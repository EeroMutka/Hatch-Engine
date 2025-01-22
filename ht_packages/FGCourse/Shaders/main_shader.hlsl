#pragma pack_matrix(row_major)

cbuffer constants : register(b0) {
    float4x4 local_to_clip;
    float4x4 local_to_world;
    float3 view_position;
    int point_light_count;
    float4 point_lights_position[16];
    float4 point_lights_emission[16];
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
    output.position = mul(float4(vertex.position, 1.0), local_to_clip);
    
    // apparently, clip space is +Y up by default in D3D11 when rendering to the swapchain. We want it to be +Y down
    output.position.y = -output.position.y;
    
    output.position_ws = mul(float4(vertex.position, 1.0), local_to_world).xyz;
    output.uv = vertex.uv;
    output.normal = vertex.normal;
	return output;
}

// -- pixel shader -----------------------------------------------------------------------------

Texture2D mytexture : register(t0);
SamplerState mysampler : register(s0);

// see https://www.shadertoy.com/view/lslGzl
float3 FilmicToneMapping(float3 color)
{
    color = max(float3(0, 0, 0), color - 0.004);
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    float3 normal = normalize(pixel.normal);
    
    float3 base_color = mytexture.Sample(mysampler, pixel.uv * 5).rgb;
    
    float3 light = 0;
    for (int i = 0; i < point_light_count; i++)
    {
        float3 point_to_light = point_lights_position[i].xyz - pixel.position_ws;
        float3 point_to_view = view_position - pixel.position_ws;
        float3 V = normalize(point_to_view);
        float3 L = normalize(point_to_light);
        float3 R = reflect(-L, normal);
        float dist = length(point_to_light);
        
        // diffuse
        light += base_color * max(dot(normal, L), 0) * point_lights_emission[i].xyz / (dist * dist);
        
        // specular
        light += base_color * 1.0 * pow(max(dot(V, R), 0.), 32.) * point_lights_emission[i].xyz;
    }
    
    //float lightness = max(dot(normal, float3(0, 0, 1)), 0.3);
    //float3 normal = normalize(cross(ddx(pixel.position_ws), ddy(pixel.position_ws)));
    return float4(FilmicToneMapping(light), 1);
    //return float4(normal*0.5  + 0.5, 1);
    //return float4(pixel.position_ws, 1);
}