static const float  C_LUT_CLAMP  = 4.0;
static const int    C_LUT_SIZE   = 128; 
static const float  C_LUT_SCALE = C_LUT_SIZE / C_LUT_CLAMP;

RWTexture1D<float> g_TonemapLUT;

Texture2D g_InputTexture;
RWTexture2D<float4> g_OutputTexture;

cbuffer TonemapParamsAlt : register(b0)
{
    float PMem;
    float aMem;
    float mMem;
    float lMem;
    float cMem;
    float bMem;
    int enableTonemapping;
}


float GTTonemap(float x)
{
    float P = 1.0;
    float a = 1.0;
    float m = 0.22;
    float l = 0.4;
    float c = 1.33;
    float b = 0.0;

    //float P = PMem;
    //float a = aMem;
    //float m = mMem;
    //float l = lMem;
    //float c = cMem;
    //float b = bMem;

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
    //return GTTonemap((float)(x * x / C_LUT_SCALE));
    return GTTonemap(x);
}

float mapSample(float x)
{
    //float x1 = sqrt(x) * C_LUT_SCALE;
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

[numthreads(32, 1, 1)]
void mainGenLUT(uint3 threadDispatchId : SV_DispatchThreadID)
{
    g_TonemapLUT[threadDispatchId.x] = mapCompress((float)threadDispatchId.x);
}

[numthreads(16, 16, 1)]
void mainTonemap(uint3 threadDispatchId : SV_DispatchThreadID)
{
    //g_OutputTexture[threadDispatchId.xy] = g_InputTexture[threadDispatchId.xy];

    //g_OutputTexture[threadDispatchId.xy] = applyTonemap(g_InputTexture[threadDispatchId.xy]);

    //float4 input = g_InputTexture[threadDispatchId.xy];
    //if(enableTonemapping)
        g_OutputTexture[threadDispatchId.xy] = float4(GTTonemap(input.x), GTTonemap(input.y), GTTonemap(input.z), input.a);
    //else
    //    g_OutputTexture[threadDispatchId.xy] = input;
}

