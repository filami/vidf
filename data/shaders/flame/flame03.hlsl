
void Iterate(inout float3 p, inout float c)
{
	const float f = RandUNorm() * 12.0;
		
	if (f <= 1.0)
	{
		float3 ft = p;

		p = 0.0;

		varPreBlur(ft, 0.5);
		varHemisphere(p, ft, 0.35);
		varZTranslate(p, ft, 0.05);

		c = 0.0;
	}
	else if (f <= 11.0)
	{
		float3 ft = p;
		ft = FlameTransform(MakeTransform(float2(0.4, 0.0), 0, 0.82), ft);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaN(p, ft, phi, r, 0.85, 2.0, -1.0);
		varHemisphere(p, ft, 0.0125);

		c = 0.85;
	}
	else if (f <= 11.5)
	{
		float3 ft = p;
		ft.xy *= float2(1, 0.15);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaN(p, ft, phi, r, 0.36, 18.0, -1.0);
		// varZCone(p, ft, r, 0.055);
		// varZTranslate(p, ft, -0.08);

		c = 0.25;
	}
	else
	{
		float3 ft = p;

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaN(p, ft, phi, r, 0.45, 10.0, -1.0);
		// varZTranslate(p, ft, 0.025);

		c = 0.35;
	}
}
