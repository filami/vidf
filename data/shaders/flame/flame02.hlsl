

void Iterate(inout float3 p, inout float c)
{	
	const float f = RandUNorm();

	if (f < 0.1)
	{
		// const float3 ft = FlameTransform(MakeTransform(0, -view.time, 1), p);
		float3 ft = p;
		ft = FlameTransform(MakeTransform(0, -view.time, 1), ft);

		const float r = length(ft.xy);
		p = 0.0;

		varBlur(p, ft, 0.1);
		varFishEye(p, ft, r, -0.1);
		varZTranslate(p, ft, -0.15);
		c = 0.65;
	}
	else if (f < 0.75)
	{
		// const float3 ft = FlameTransform(MakeTransform(float2(0.25, 0.5), degToRad * 45, 1), p);
		float3 ft = p;
		ft = FlameTransform(MakeTransform(float2(0.5, 0.25), 0, 1), ft);
		ft = FlameTransform(MakeTransform(0, -degToRad * 45 * view.time, 1), ft);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaN(p, ft, phi, r, 0.35, 3.0, -0.5);
		varZScale(p, ft, 0.25);
		// c = 0.7;
		c = sin(view.time * 0.2) * 0.5 + 0.5;
	}
	else
	{
		// const float3 ft = FlameTransform(MakeTransform(float2(-1.0, 0), 0, 4), p);
		float3 ft = p;
		// ft = FlameTransform(MakeTransform(0, view.time, 1), ft);
		// ft = FlameTransform(MakeTransform(float2(-1.0, 0), 0, 4), ft);

		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		varJuliaScope(p, ft, phi, r, 1.0, -2.0, 1.0);
		varZCone(p, ft, r, 0.025);
		varZScale(p, ft, -0.15);
		// c = 0.25;
		c = sin(view.time * 2.0) * 0.25 + 0.75;
	}
}
