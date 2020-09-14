//	VQE
//	Copyright(C) 2020  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "Lighting.hlsl"

//---------------------------------------------------------------------------------------------------
//
// DATA
//
//---------------------------------------------------------------------------------------------------
struct VSInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float3 tangent  : TANGENT;
	float2 uv       : TEXCOORD0;
#ifdef INSTANCED
	uint instanceID : SV_InstanceID;
#endif
};

struct PSInput
{
    float4 position    : SV_POSITION;
	float3 worldPos    : POSITION1;
	float3 vertNormal  : COLOR0;
	float3 vertTangent : COLOR1;
	float2 uv          : TEXCOORD0;
#ifdef INSTANCED
	uint instanceID    : SV_InstanceID;
#endif
};

#ifdef INSTANCED
	#ifndef INSTANCE_COUNT
	#define INSTANCE_COUNT 50 // 50 instances assumed per default, this should be provided by the app
	#endif
#endif

//---------------------------------------------------------------------------------------------------
//
// RESOURCE BINDING
//
//---------------------------------------------------------------------------------------------------
cbuffer CBPerFrame : register(b0)
{
	PerFrameData cbPerFrame;
}
cbuffer CBPerView : register(b1)
{
	PerViewData cbPerView;
}
cbuffer CBPerObject : register(b2)
{
#ifdef INSTANCED
	PerObjectData cbPerObject[INSTANCE_COUNT];
#else
	PerObjectData cbPerObject;
#endif
}

Texture2D texDiffuse        : register(t0);
Texture2D texNormals        : register(t1);
Texture2D texEmissive       : register(t2);
Texture2D texAlphaMask      : register(t3);
Texture2D texMetalness      : register(t4);
Texture2D texRoughness      : register(t5);
Texture2D texOcclRoughMetal : register(t6);
Texture2D texLocalAO        : register(t7);


SamplerState LinearSampler : register(s0);
SamplerState PointSampler  : register(s1);
SamplerState AnisoSampler  : register(s2);

Texture2D        texDirectionalLightShadowMap : register(t10);
Texture2DArray   texSpotLightShadowMaps       : register(t13);
TextureCubeArray texPointLightShadowMaps      : register(t19);

//---------------------------------------------------------------------------------------------------
//
// KERNELS
//
//---------------------------------------------------------------------------------------------------
PSInput VSMain(VSInput vertex)
{
	PSInput result;
	
#ifdef INSTANCED
	result.position    = mul(cbPerObject[vertex.instanceID].matWorldViewProj, float4(vertex.position, 1.0f));
	result.vertNormal  = mul(cbPerObject[vertex.instanceID].matNormal, vertex.normal );
	result.vertTangent = mul(cbPerObject[vertex.instanceID].matNormal, vertex.tangent);
	result.worldPos    = mul(cbPerObject[vertex.instanceID].matWorld, float4(vertex.position, 1.0f));
#else
	result.position    = mul(cbPerObject.matWorldViewProj, float4(vertex.position, 1.0f));
	result.vertNormal  = mul(cbPerObject.matNormal, vertex.normal );
	result.vertTangent = mul(cbPerObject.matNormal, vertex.tangent);
	result.worldPos    = mul(cbPerObject.matWorld, float4(vertex.position, 1.0f));
#endif
	result.uv          = vertex.uv;
	
	return result;
}


float4 PSMain(PSInput In) : SV_TARGET
{
	const float2 uv = In.uv;
	const int TEX_CFG = cbPerObject.materialData.textureConfig;
	
	float4 AlbedoAlpha = texDiffuse  .Sample(AnisoSampler, uv);
	float3 Normal      = texNormals  .Sample(AnisoSampler, uv).rgb;
	float3 Emissive    = texEmissive .Sample(LinearSampler, uv).rgb;
	float3 Metalness   = texMetalness.Sample(AnisoSampler, uv).rgb;
	float3 Roughness   = texRoughness.Sample(AnisoSampler, uv).rgb;
	float3 OcclRghMtl  = texOcclRoughMetal.Sample(AnisoSampler, uv).rgb;
	float LocalAO      = texLocalAO.Sample(AnisoSampler, uv).r;
	
	if (HasDiffuseMap(TEX_CFG) && AlbedoAlpha.a < 0.01f)
		discard;
	
	// ensure linear space
	AlbedoAlpha.xyz = SRGBToLinear(AlbedoAlpha.xyz);
	Emissive        = SRGBToLinear(Emissive);
	
	// read textures/cbuffer & assign sufrace material data
	float ao = cbPerFrame.fAmbientLightingFactor;
	BRDF_Surface SurfaceParams = (BRDF_Surface)0;
	SurfaceParams.diffuseColor      = HasDiffuseMap(TEX_CFG)   ? AlbedoAlpha.rgb : cbPerObject.materialData.diffuse;
	SurfaceParams.specularColor     = float3(1,1,1);
	SurfaceParams.emissiveColor     = HasEmissiveMap(TEX_CFG)  ? Emissive        : cbPerObject.materialData.emissiveColor;
	SurfaceParams.emissiveIntensity = cbPerObject.materialData.emissiveIntensity;
	
	const float3  N = normalize(In.vertNormal);
	const float3  T = normalize(In.vertTangent);
	SurfaceParams.N = length(Normal) < 0.01 ? N : UnpackNormal(Normal, N, T);
	
	bool bReadsRoughnessMapData = HasRoughnessMap(TEX_CFG) || HasOcclusionRoughnessMetalnessMap(TEX_CFG);
	bool bReadsMetalnessMapData =  HasMetallicMap(TEX_CFG) || HasOcclusionRoughnessMetalnessMap(TEX_CFG);
	
	if (!bReadsRoughnessMapData) SurfaceParams.roughness = cbPerObject.materialData.roughness;
	if (!bReadsMetalnessMapData) SurfaceParams.metalness = cbPerObject.materialData.metalness;
	if (HasAmbientOcclusionMap(TEX_CFG)) ao *= LocalAO;
	if (HasRoughnessMap(TEX_CFG) ) SurfaceParams.roughness = Roughness;
	if (HasMetallicMap(TEX_CFG)  ) SurfaceParams.metalness = Metalness;
	if (HasOcclusionRoughnessMetalnessMap(TEX_CFG))
	{
		ao *= OcclRghMtl.r;
		SurfaceParams.roughness = OcclRghMtl.g;
		SurfaceParams.metalness = OcclRghMtl.b;
	}
	
	// lighting & surface parameters (World Space)
	const float3 P = In.worldPos;
	const float3 V = normalize(cbPerView.CameraPosition - P);
	const float2 screenSpaceUV = In.position.xy / cbPerView.ScreenDimensions;
	
	
	// illumination accumulators
	float3 I_total = 
	/* ambient  */   SurfaceParams.diffuseColor  * ao
	/* Emissive */ + SurfaceParams.emissiveColor * SurfaceParams.emissiveIntensity * 10.0f
	;
	
	float3 IEnv = 0.0f.xxx; // environment lighting illumination
	
	// -------------------------------------------------------------------------------------------------------
	
	// Non-shadowing lights
	for (int p = 0; p < cbPerFrame.Lights.numPointLights; ++p)
	{
		I_total += CalculatePointLightIllumination(cbPerFrame.Lights.point_lights[p], SurfaceParams, P, V);
	}
	for (int s = 0; s < cbPerFrame.Lights.numSpotLights; ++s)
	{
		I_total += CalculateSpotLightIllumination(cbPerFrame.Lights.spot_lights[s], SurfaceParams, P, V);
	}
	
	
	// Shadow casters - POINT
	for (int pc = 0; pc < cbPerFrame.Lights.numPointCasters; ++pc)
	{
		const float2 PointLightShadowMapDimensions = cbPerFrame.f2PointLightShadowMapDimensions;
		PointLight l = cbPerFrame.Lights.point_casters[pc];
		const float  D = length   (l.position - P);

		if(D < l.range)
		{
			const float3 L = normalize(l.position - P);
			const float3 Lw = (l.position - P);
			ShadowTestPCFData pcfData = (ShadowTestPCFData) 0;
			pcfData.depthBias           = l.depthBias;
			pcfData.NdotL               = saturate(dot(SurfaceParams.N, L));
			pcfData.viewDistanceOfPixel = length(P - cbPerView.CameraPosition);
		
			I_total += CalculatePointLightIllumination(cbPerFrame.Lights.point_casters[pc], SurfaceParams, P, V)
					* OmnidirectionalShadowTestPCF(pcfData, texPointLightShadowMaps, PointSampler, PointLightShadowMapDimensions, pc, Lw, l.range);
		}
	}
	
	// Shadow casters - SPOT
	for (int sc = 0; sc < cbPerFrame.Lights.numSpotCasters; ++sc)
	{
		SpotLight l = cbPerFrame.Lights.spot_casters[sc];
		const float3 L = normalize(l.position - P);
		const float2 SpotLightShadowMapDimensions = cbPerFrame.f2SpotLightShadowMapDimensions;
		
		ShadowTestPCFData pcfData = (ShadowTestPCFData) 0;
		pcfData.depthBias           = l.depthBias;
		pcfData.NdotL               = saturate(dot(SurfaceParams.N, L));
		pcfData.lightSpacePos       = mul(cbPerFrame.Lights.shadowViews[sc], float4(P, 1));
		pcfData.viewDistanceOfPixel = length(P - cbPerView.CameraPosition);
		
		I_total += CalculateSpotLightIllumination(cbPerFrame.Lights.spot_casters[sc], SurfaceParams, P, V)
			  * ShadowTestPCF(pcfData, texSpotLightShadowMaps, PointSampler, SpotLightShadowMapDimensions, sc);
	}
	
	
	// Directional light
	{
		DirectionalLight l = cbPerFrame.Lights.directional;
		if (l.enabled)
		{
			ShadowTestPCFData pcfData = (ShadowTestPCFData) 0;
			float ShadowingFactor = 1.0f; // no shadows
			if (l.shadowing)
			{
				const float3 L = normalize(-l.lightDirection);
				pcfData.lightSpacePos = mul(cbPerFrame.Lights.shadowViewDirectional, float4(P, 1));
				pcfData.NdotL = saturate(dot(SurfaceParams.N, L));
				pcfData.depthBias = l.depthBias;
				ShadowingFactor = ShadowTestPCF_Directional(pcfData, texDirectionalLightShadowMap, PointSampler, cbPerFrame.f2DirectionalLightShadowMapDimensions, cbPerFrame.Lights.shadowViewDirectional);
			}
			
			I_total += CalculateDirectionalLightIllumination(l, SurfaceParams, V) * ShadowingFactor;
		}
	}
	
	return float4(I_total, 1);
	//return float4(SurfaceParams.N, 1); // debug line
	//return float4(SurfaceParams.roughness.xxx, 1); // debug line
	//return float4(SurfaceParams.metalness.xxx, 1); // debug line
}