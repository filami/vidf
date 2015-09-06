#pragma once

#include "Vector2.h"
#include "Vector3.h"


namespace vidf {


	class UniformCircle {
	public:
		UniformCircle() {}

		template<class Engine>
		Vector2f operator()(Engine& eng) {
			using std::sqrt;
			Vector2f res(
				mNormalDist(eng),
				mNormalDist(eng));
			return Normalize(res);
		}

	private:
		NormalDistribution<real32> mNormalDist;
	};


	class UniformSphere {
	public:
		UniformSphere() {}

		template<class Engine>
		Vector3f operator()(Engine& eng) {
			using std::sqrt;
			Vector3f res(
				mNormalDist(eng),
				mNormalDist(eng),
				mNormalDist(eng));
			return Normalize(res);
		}

	private:
		NormalDistribution<real32> mNormalDist;
	};

}
