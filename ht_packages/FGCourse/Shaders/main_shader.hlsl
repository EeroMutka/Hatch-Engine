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
    float use_specular_value;
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
Texture2D t_specular : register(t1);
Texture2D t_dir_light_depth_map : register(t2);
SamplerState s_nearest_wrap : register(s0);
SamplerState s_linear_wrap : register(s1);
SamplerComparisonState s_percentage_closer : register(s2);

// see https://www.shadertoy.com/view/lslGzl
float3 FilmicToneMapping(float3 color)
{
    color = max(float3(0, 0, 0), color - 0.004);
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

float InverseLerp(float a, float b, float x) {
    return (x - a) / (b - a);
}

float4 pixel_shader(PixelInput pixel) : SV_TARGET {
    float3 normal = normalize(pixel.normal);
    
    float3 base_color = pow(t_base_color.Sample(s_linear_wrap, pixel.uv).rgb, 2.2); // convert srgb to linear
    float specular = use_specular_value < 0.0 ? t_specular.Sample(s_linear_wrap, pixel.uv).r : use_specular_value;
    
    float3 point_to_view = view_position - pixel.position_ws;
    float3 V = normalize(point_to_view);
    
    float3 light = 0;
    
    const float specular_intensity = 5.0;
    const float specular_diffuse_drop = 0.5;
    
    for (int i = 0; i < point_light_count; i++)
    {
        float3 point_to_light = point_lights_position[i].xyz - pixel.position_ws;
        float3 L = normalize(point_to_light);
        float3 R = reflect(-L, normal);
        float dist = length(point_to_light);
        
        // diffuse
        float3 light_contribution = (1.0 - specular_diffuse_drop*specular) * max(dot(normal, L), 0);
        
        // specular
        light_contribution += specular_intensity * specular * pow(max(dot(V, R), 0.), 32.);
        
        light_contribution *= point_lights_emission[i].xyz / (dist * dist);
        
        if (dist < point_lights_emission[i].w) // is in radius?
            light += light_contribution;
    }
    
    for (int i2 = 0; i2 < spot_light_count; i2++)
    {
        float3 point_to_light = spot_lights_position[i2].xyz - pixel.position_ws;
        float3 L = normalize(point_to_light);
        float3 R = reflect(-L, normal);
        float dist = length(point_to_light);
        
        float cos_inner_angle = spot_lights_direction[i2].w;
        float cos_outer_angle = spot_lights_position[i2].w;
        float directional_factor = InverseLerp(cos_outer_angle, cos_inner_angle, max(dot(L, -spot_lights_direction[i2].xyz), 0));
        directional_factor = clamp(directional_factor, 0.f, 1.f);
        
        // diffuse
        float3 light_contribution = (1.0 - specular_diffuse_drop*specular) * max(dot(normal, L), 0);
        
        // specular
        light_contribution += specular_intensity * specular * pow(max(dot(V, R), 0.), 32.);
        
        light_contribution *= spot_lights_emission[i2].xyz * directional_factor / (dist * dist);
        
        if (dist < spot_lights_emission[i2].w) // is in radius?
            light += light_contribution;
    }
    
    {
        float3 point_in_dir_shadow = mul(float4(pixel.position_ws + normal*0.1, 1.0), world_to_dir_shadow).xyz;
        point_in_dir_shadow.xy = point_in_dir_shadow.xy * 0.5 + 0.5;
        
        float shadow_map_w, shadow_map_h;
        t_dir_light_depth_map.GetDimensions(shadow_map_w, shadow_map_h);
        
        float off = 0.5f / shadow_map_w; // offset
        float shadowness =
            t_dir_light_depth_map.SampleCmpLevelZero(s_percentage_closer, point_in_dir_shadow.xy + float2(off, off), point_in_dir_shadow.z).r +
            t_dir_light_depth_map.SampleCmpLevelZero(s_percentage_closer, point_in_dir_shadow.xy + float2(-off, off), point_in_dir_shadow.z).r +
            t_dir_light_depth_map.SampleCmpLevelZero(s_percentage_closer, point_in_dir_shadow.xy + float2(-off, -off), point_in_dir_shadow.z).r +
            t_dir_light_depth_map.SampleCmpLevelZero(s_percentage_closer, point_in_dir_shadow.xy + float2(off, -off), point_in_dir_shadow.z).r;
        shadowness /= 4.f;
        
        {
            float3 L = normalize(-directional_light_dir);
            float3 R = reflect(-L, normal);
            
            // diffuse
            float3 light_contribution = (1.0 - specular_diffuse_drop*specular) * max(dot(normal, L), 0);
            
            // specular
            light_contribution += specular_intensity * specular * pow(max(dot(V, R), 0.), 32.);
            
            light_contribution *= directional_light_emission;
            
            light += shadowness * light_contribution;
        }
    }
    
    // ambient light
    light += 0.5*float3(0.12, 0.15, 0.17);
    
    light *= base_color;
    
    //float lightness = max(dot(normal, float3(0, 0, 1)), 0.3);
    //float3 normal = normalize(cross(ddx(pixel.position_ws), ddy(pixel.position_ws)));
    //return float4(normal*0.5  + 0.5, 1);
    //return float4(specular, specular, specular, 1);
    return float4(FilmicToneMapping(light), 1);
}