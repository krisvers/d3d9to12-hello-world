struct vinput_t
{
    float3 pos : POSITION;
    float3 color : COLOR;
};

struct voutput_t
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

voutput_t vsmain(vinput_t input)
{
    voutput_t output;
    output.pos = float4(input.pos, 1.0f);
    output.color = input.color;
    return output;
}

float4 psmain(voutput_t input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}