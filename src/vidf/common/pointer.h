#pragma once


namespace vidf
{


	template<typename SmartType>
	class Pointer
	{
	public:
		Pointer()
			:	smartPointer(0)
		{
		}

		template<typename SmartType2>
		Pointer(SmartType2* _smartPointer)
			:	smartPointer(_smartPointer)
		{
		}

		Pointer(const Pointer<SmartType>& pointer)
			:	smartPointer(pointer.smartPointer)
		{
			if (smartPointer)
				smartPointer->AddRef();
		}

		~Pointer()
		{
			if (smartPointer)
				smartPointer->Release();
		}

		template<typename SmartType2>
		void operator=(SmartType2* _smartPointer)
		{
			if (smartPointer)
				smartPointer->Release();
			smartPointer = _smartPointer;
			if (smartPointer)
				smartPointer->AddRef();
		}

		void operator=(const Pointer<SmartType>& pointer)
		{
			if (smartPointer)
				smartPointer->Release();
			smartPointer = pointer.smartPointer;
			if (smartPointer)
				smartPointer->AddRef();
		}

		SmartType* operator-> () {return smartPointer;}
		const SmartType* operator-> () const {return smartPointer;}
		SmartType& operator* () {return &smartPointer;}
		const SmartType& operator* () const {return &smartPointer;}

		operator SmartType*() {return smartPointer;}
		operator const SmartType*() const {return smartPointer;}

		SmartType*& Get() {return smartPointer;}
		const SmartType*& Get() const {return smartPointer;}

		operator bool () const {return smartPointer != 0;}

	private:
		SmartType* smartPointer;
	};



	template<typename Resource>
	void SafeRelease(Resource*& resource)
	{
		if (resource)
			resource->Release();
		resource = 0;
	}



	template<typename Resource>
	unsigned int GetRefCount(Resource* resource)
	{
		resource->AddRef();
		unsigned int result = (unsigned int) resource->Release();
		return result;
	}



	template<typename Resource>
	unsigned int GetRefCount(Pointer<Resource>& resource)
	{
		return GetRefCount(resource.Get());
	}


}
