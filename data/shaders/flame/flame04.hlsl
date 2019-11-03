
static const float focalDist = 3.0;
static const float cocSize = 0.045;

static const float brightness = 5.0;
static const float saturation = 0.95;
static const float vibrance = 1.6;
static const float gamma = 3.0;

void Iterate(inout float3 p, inout float c)
{
	const float f = RandUNorm() * 15.0;

	if (f <= 12.0)
	{
		float3 ft = p;
		ft = FlameTransform(MakeTransform(float2(0.85, 0.0), 0, 1), ft);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaN(p, ft, phi, r, 0.85, 4.0, -1.0);
		varZScale(p, ft, -0.5);
		varZCone(p, ft, r, -0.2);

		c = 0.32;
	}
	else if (f <= 14.0)
	{
		float3 ft = p;
		ft = FlameTransform(MakeTransform(float2(0.0, 0.0), pi*0.5, 0.25), ft);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaScope(p, ft, phi, r, 0.75, 6.0, 1.0);
		varZCone(p, ft, r, 0.55);
		varZScale(p, ft, -0.25);

		c = 0.4;
	}
	else
	{
		float3 ft = p;

		p = 0.0;
				
		varBlur(p, ft, 0.35);
		varHemisphere(p, ft, 0.1);
		varZTranslate(p, ft, 0.15);

		c = 0.8;
	}
}
