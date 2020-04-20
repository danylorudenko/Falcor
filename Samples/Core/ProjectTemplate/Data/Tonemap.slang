static const float C_LUT_MAX_VALUE          = 4.0;
static const float C_LUT_SIZE               = 128.0;
static const float C_LUT_COMPRESS_FACTOR    = 1.0 / (C_LUT_SIZE / C_LUT_MAX_VALUE);
static const float C_LUT_SAMPLE_FACTOR      = 1.0 / C_LUT_MAX_VALUE;


#if defined(LUT_GENERATION)

RWTexture1D<float>  g_TonemapLUT;

cbuffer GTParams : register(b0)
{
    float P;
    float a;
    float m;
    float l;
    float c;
    float b;
}

float GTTonemap(float x)
{
    float l0 = (P - m) * l / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);

    float L = m + a * (x - m);
    float T = m * pow(x / m, c);
    float S = P - (P - S1) * exp(-C2 * (x - S0) / P);
    float w0 = 1 - smoothstep(0.0f, m, x);
    float w2 = (x < m + l) ? 0 : 1;
    float w1 = 1 - w0 - w2;
    return (float)(T * w0 + L * w1 + S * w2);
}

float mapCompress(float x)
{
    return GTTonemap(x * C_LUT_COMPRESS_FACTOR);
}

[numthreads(32, 1, 1)]
void mainGenLUT(uint3 threadDispatchId : SV_DispatchThreadID)
{
    g_TonemapLUT[threadDispatchId.x] = mapCompress((float)threadDispatchId.x);
}
#endif // LUT_GENERATION

//////////////////////////////////////////////////////////

#ifdef TONEMAPPING_PASS
SamplerState        g_LUTSamplerState;
Texture1D           g_TonemapLUT;

Texture2D           g_InputTexture;
RWTexture2D<float4> g_OutputTexture;

cbuffer TonemapPassParams : register(b1)
{
    int enableTonemapping;
}

float mapSample(float x)
{
    float xS = x * C_LUT_SAMPLE_FACTOR;
    return g_TonemapLUT.SampleLevel(g_LUTSamplerState, xS, 0).x;
}

float4 applyTonemap(float4 linearRGB)
{
    float4 result;

    result.r = mapSample(linearRGB.r);
    result.g = mapSample(linearRGB.g);
    result.b = mapSample(linearRGB.b);
    result.a = linearRGB.a;

    return result;
}

[numthreads(16, 16, 1)]
void mainTonemap(uint3 threadDispatchId : SV_DispatchThreadID)
{
    float4 input = g_InputTexture[threadDispatchId.xy];

    float4 result;
    if(enableTonemapping != 0)
    {
        result = applyTonemap(input);
    }
    else
    {
        result = input;
    }

    float4 sRGB = pow(result, 1.0 / 2.2);
    g_OutputTexture[threadDispatchId.xy] = sRGB;
}
#endif // TONEMAPPING_PASS


