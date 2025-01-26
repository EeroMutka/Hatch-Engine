#pragma pack_matrix(row_major)

//#include "shader_constants.h"

cbuffer constants : register(b0) {
    float4x4 local_to_clip;
    float4x4 local_to_world;
    float4x4 world_to_dir_shadow;
    
    float3 view_position;
    int _pad1;
    
    int point_light_count;
    int spot_light_count;
    int _pad2;
    int _pad3;
    
    float3 directional_light_dir;
    int _pad4;
    float3 directional_light_emission;
    int _pad5;
    
    float4 point_lights_position[16];
    float4 point_lights_emission[16];

    float4 spot_lights_direction[16];
    float4 spot_lights_position[16];
    float4 spot_lights_emission[16];
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
    output.normal = mul(float4(vertex.normal, 0.0), local_to_world).xyz;
	return output;
}

// -- pixel shader -----------------------------------------------------------------------------

Texture2D t_base_color : register(t0);
Texture2D t_dir_light_depth_map : register(t1);
SamplerState s_nearest_wrap : register(s0);

// see https://www.shadertoy.com/view/lslGzl
float3 FilmicToneMapping(float3 color)
{
    color = max(float3(0, 0, 0), color - 0.004);
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    float3 normal = normalize(pixel.normal);
    float3 base_color = t_base_color.Sample(s_nearest_wrap, pixel.uv * 5).rgb;
    
    float3 point_to_view = view_position - pixel.position_ws;
    float3 V = normalize(point_to_view);
    
    float3 light = 0;
    for (int i = 0; i < point_light_count; i++)
    {
        float3 point_to_light = point_lights_position[i].xyz - pixel.position_ws;
        float3 L = normalize(point_to_light);
        float3 R = reflect(-L, normal);
        float dist = length(point_to_light);
        
        // diffuse
        float3 light_contribution = max(dot(normal, L), 0);
        
        // specular
        light_contribution += 1.0 * pow(max(dot(V, R), 0.), 32.);
        
        light_contribution *= point_lights_emission[i].xyz / (dist * dist);
        
        light += light_contribution;
    }
    
    for (int i = 0; i < spot_light_count; i++)
    {
        float3 point_to_light = spot_lights_position[i].xyz - pixel.position_ws;
        float3 L = normalize(point_to_light);
        float3 R = reflect(-L, normal);
        float dist = length(point_to_light);
        
        // diffuse
        float3 light_contribution = max(dot(normal, L), 0);
        
        // specular
        light_contribution += 1.0 * pow(max(dot(V, R), 0.), 32.);
        
        light_contribution *= spot_lights_emission[i].xyz * max(dot(L, -spot_lights_direction[i].xyz), 0) / (dist * dist);
        
        light += light_contribution;
    }
    
    {
        float3 point_in_dir_shadow = mul(float4(pixel.position_ws + normal*0.1, 1.0), world_to_dir_shadow).xyz;
        point_in_dir_shadow.xy = point_in_dir_shadow.xy * 0.5 + 0.5;
        
        // so how do we compute what the actual value should be?
        if (t_dir_light_depth_map.Sample(s_nearest_wrap, point_in_dir_shadow.xy).r > point_in_dir_shadow.z)
        {
            float3 L = normalize(-directional_light_dir);
            float3 R = reflect(-L, normal);
            
            // diffuse
            float3 light_contribution = max(dot(normal, L), 0);
            
            // specular
            //light_contribution += 1.0 * pow(max(dot(V, R), 0.), 32.);
            
            light_contribution *= directional_light_emission;
            
            light += light_contribution;
        }
    }
    
    // ambient light
    light += 0.3*float3(0.12, 0.15, 0.17);
    
    light *= base_color;
    
    //float lightness = max(dot(normal, float3(0, 0, 1)), 0.3);
    //float3 normal = normalize(cross(ddx(pixel.position_ws), ddy(pixel.position_ws)));
    //return float4(normal*0.5  + 0.5, 1);
    //return float4(pixel.position_ws, 1);
    return float4(FilmicToneMapping(light), 1);
}