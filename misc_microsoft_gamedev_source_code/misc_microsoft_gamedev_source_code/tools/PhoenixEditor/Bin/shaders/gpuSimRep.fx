

float4x4 worldViewProj	: WorldViewProjection;	// This matrix will be loaded by the application


/////////////////////////////////////////////
/////////////////////////////////////////////
//DEPTH PEELING (for AO GEN!)

struct VS_OUTPUT_DEPTH
{
    float4 hposition	: POSITION;
    
    float4 rPos			: TEXCOORD5;		//same as hposition
};

VS_OUTPUT_DEPTH myvsDepthPeel(   float4 vPosition : POSITION)
{
    VS_OUTPUT_DEPTH OUT;

	float4 tPos = mul( vPosition, worldViewProj );
	OUT.hposition = tPos;
	OUT.rPos = tPos;
	
	return OUT;
}


float4 mypsDepthOnly( VS_OUTPUT_DEPTH IN ) : COLOR
{
	float myDepth =  IN.rPos.z / IN.rPos.w;
	//myDepth+=0.009;
    return myDepth;
}


technique Technique0
{
     pass DepthOnly
    {
		VertexShader = compile vs_3_0 myvsDepthPeel();
		PixelShader  = compile ps_3_0 mypsDepthOnly();
    }   
}


