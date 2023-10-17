#pragma pack_matrix(row_major)

Texture2D colorTexture : register(t0);
SamplerState filter : register(s0)

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PS_IN input) : SV_TARGET
{
	return colorTexture.Sample(filter, input.uv);
}