

void Iterate(inout float3 p, inout float c)
{
	const float f = RandUNorm();

	if (f < 0.1)
	{
		const float3 ft = float3(p.xy*0.75, p.z);
		const float r = length(ft.xy);
		p = 0.0;

		varFlatten(p);
		varLinear(p, ft, 1.0);
		varBlur(p, ft, 0.015);
		c = 0.05;
	}
	else if (f < 0.65)
	{
		const float3 ft = float3(p.xy * 0.75 + float2(-0.25, 0.25), p.z);
		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		// varZTranslate(p, ft, 0.05);
		// varZCone(p, ft, r, 0.1);
		varJuliaN(p, ft, phi, r, 0.65, 4.0, -1.0);
		c = 0.5;
	}
	else if (f < 0.85)
	{
		const float3 ft = float3(p.xy * float2(-6, 0.5) + float2(-0.2, 0.5), p.z);
		const float r = length(ft.xy);
		const float phi = atan2(ft.y, ft.x);
		p = 0.0;

		// varZCone(p, ft, r, 0.005);
		varJuliaScope(p, ft, phi, r, 0.75, -8.0, 2.0);
		c = 0.9;
	}
	else
	{
		const float3 ft = float3(p.xy * 1.5, p.z);
		const float r = length(ft.xy);
		p = 0.0;

		// varZCone(p, ft, r, -0.05);
		varZTranslate(p, ft, -0.05);
		varZScale(p, ft, 0.25);
		varLinear(p, ft, 0.5);
		varEyeFish(p, ft, r, 0.5);
		c = 0.4;
	}
}
