Texture2D g_InputTexture;
RWTexture2D<float4> g_OutputTexture;

[numthreads(16, 16, 1)]
void main(uint3 threadDispatchId : SV_DispatchThreadID)
{
    g_OutputTexture[threadDispatchId.xy] = g_InputTexture[threadDispatchId.xy] + 0.2;
}
