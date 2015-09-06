#pragma once

namespace vidf
{


	struct Class
	{
	public:
		Class(const char* _className, Class* _baseClass)
			: baseClass(_baseClass)
			, className(_className) {}
		const Class* GetBaseClass() const {return baseClass;}
		const char* GetClassName() const {return className;}
		bool IsA(const Class* type) const;
	private:
		Class* baseClass;
		const char* className;
	};


	#define DECLARE_VIDF_RTTI_ROOT(ObjectClass)								\
		public:																\
			static vidf::Class* GetClassType() {							\
				static vidf::Class objectType(#ObjectClass, 0);				\
				return &objectType;											\
			}																\
			virtual const vidf::Class* GetObjectType() const {				\
				return ObjectClass::GetClassType();							\
			}																\
			bool IsA(const vidf::Class* type) const {						\
				const vidf::Class* thisType = this->GetObjectType();		\
				return thisType->IsA(type);									\
			}


	#define DECLARE_VIDF_RTTI(ObjectClass, BaseClass)										\
		public:																				\
			static vidf::Class* GetClassType() {											\
				static vidf::Class objectType(#ObjectClass, BaseClass::GetClassType());		\
				return &objectType;															\
			}																				\
			virtual const vidf::Class* GetObjectType() const {								\
				return ObjectClass::GetClassType();											\
			}


	template <typename R>
	struct vidf_cast_impl {
		template <typename A>
		R operator () (A value)
		{
			const Class* type = R::GetClassType();
			if (!value.IsA(type))
				return R();
			return value;
		}
	};



	template <typename R>
	struct vidf_cast_impl <R*>
	{
		template <typename A> 
		R* operator () (A* ptr)
		{
			if (!ptr)
				return 0;
			const Class* type = R::GetClass();
			if (!ptr->IsA(type))
				return 0;
			return reinterpret_cast<R*>(ptr);
		}
	};



	template <typename R, typename A>
	R vidf_cast(A ptr)
	{
		return vidf_cast_impl<R>()(ptr);
	}



	inline bool Class::IsA(const Class* type) const
	{
		const Class* cmpType = this;
		while (cmpType)
		{
			if (cmpType == type)
				return true;
			cmpType = cmpType->baseClass;
		}
		return false;
	}


}
