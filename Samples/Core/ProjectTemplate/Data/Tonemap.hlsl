static const float  C_LUT_CLAMP  = 4.0;
static const int    C_LUT_SIZE   = 128; 
static const float  C_LUT_SCALE = C_LUT_SIZE / C_LUT_CLAMP;

RWTexture1D<float>  g_TonemapLUT;

#ifdef LUT_GENERATION
cbuffer TonemapLUTParams : register(b0)
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
    //float P = 1.0;
    //float a = 1.0;
    //float m = 0.22;
    //float l = 0.4;
    //float c = 1.33;
    //float b = 0.0;

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
    return GTTonemap(x);
}

[numthreads(32, 1, 1)]
void mainGenLUT(uint3 threadDispatchId : SV_DispatchThreadID)
{
    g_TonemapLUT[threadDispatchId.x] = mapCompress((float)threadDispatchId.x);
}
#endif // LUT_GENERATION

//////////////////////////////////////////////////////////

#ifdef TONEMAPPING_PASS
Texture2D           g_InputTexture;
RWTexture2D<float4> g_OutputTexture;

cbuffer TonemapPassParams : register(b0)
{
    int enableTonemapping;
}

float mapSample(float x)
{
    float x1 = x;

    float f = floor(x1);
    float fSample = g_TonemapLUT[f].x;

    float c = ceil(x1);
    float cSample = g_TonemapLUT[c].x;

    float toFloor = x1 - f;

    return smoothstep(fSample, cSample, toFloor);
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
    //float testVar = enableTonemapping;
    if(enableTonemapping != 0)
        g_OutputTexture[threadDispatchId.xy] = applyTonemap(input)/* * testVar*/;
    else
        g_OutputTexture[threadDispatchId.xy] = input;
}
#endif // TONEMAPPING_PASS


