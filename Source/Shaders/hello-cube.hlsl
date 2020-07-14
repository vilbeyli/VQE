//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

struct PerFrame{};  // TODO
struct PerView {};  // TODO
struct PerObject{}; // TODO

cbuffer CBuffer : register(b0)
{
	float4x4 matModelViewProj;
}


Texture2D    texColor;
SamplerState Sampler;

PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
    PSInput result;

	result.position = mul(matModelViewProj, position);
    result.color = color;
	result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 ColorTex  = texColor.SampleLevel(Sampler, input.uv, 0).rgb;
	float3 ColorVert = input.color * 0.8f; // Dim the vert colors a bit... they're too bright and look uglier.
	float3 Color = ColorVert * ColorTex;
	return float4(Color, 1);
}