#pragma once

#include "types.h"


namespace vidf
{


	template<typename T, typename IdxT=uint16, typename SaltT=uint16>
	class SaltedArray
	{
	public:
		typedef typename std::vector<T> Elements;
		typedef typename std::vector<uint8> States;
		typedef typename std::vector<T>::iterator ElementIt;
		typedef typename std::vector<uint8>::iterator StateIt;

		struct SaltId
		{
			SaltId()
				:	index(0)
				,	salt(0) {}
			IdxT index;
			SaltT salt;

			bool operator== (SaltId other) const {return index==other.index && salt==other.salt;}
		};

		struct iterator
		{
			iterator() {}
			iterator(ElementIt _elemIt, ElementIt _endIt, StateIt _stateIt)
				:	elemIt(_elemIt)
				,	endIt(_endIt)
				,	stateIt(_stateIt) {}
			bool operator != (iterator it) const
			{
				return elemIt != it.elemIt;
			}
			void operator++ ()
			{
				while (++elemIt!=endIt && *++stateIt==0);
			}
			void operator++ (int)
			{
				while (++elemIt!=endIt && *++stateIt==0);
			}
			T& operator-> ()
			{
				return *elemIt;
			}
			T& operator*() const
			{
				return *elemIt;
			}

		private:
			ElementIt elemIt;
			ElementIt endIt;
			StateIt stateIt;
		};

	public:
		SaltedArray(int arraySize)
			:	elements(arraySize)
			,	salts(arraySize, SaltT(0))
			,	state(arraySize, 0)
		{
			for (size_t i = 0; i < arraySize; ++i)
				emptySlots.push(IdxT(arraySize-i-1));
		}

		SaltId Add(const T& element)
		{
			assert(!emptySlots.empty());

			SaltId newId;
			newId.index = emptySlots.top();
			emptySlots.pop();

			newId.salt = ++salts[newId.index];

			elements[newId.index] = element;
			state[newId.index] = 1;

			return newId;
		}

		void Remove(SaltId elementId)
		{
			if (IsValidId(elementId))
			{
				elements[elementId.index] = T();
				state[elementId.index] = 0;
				emptySlots.push(elementId.index);
			}
		}

		bool GetElement(SaltId elementId, T* element, T defaultVal=T(0)) const
		{
			assert(element);
			bool valid = IsValidId(elementId);
			*element = valid ? elements[elementId.index] : defaultVal;
			return valid;
		}

		iterator begin()
		{
			auto elemIt = elements.begin();
			auto endIt = elements.end();
			auto stateIt = state.begin();
			while (elemIt!=endIt && *stateIt==0)
				{++elemIt; ++stateIt;}
			return iterator(elemIt, endIt, stateIt);
		}

		iterator end()
		{
			return iterator(elements.end(), elements.end(), state.end());
		}

	private:
		bool IsValidId(SaltId elementId) const
		{
			return salts[elementId.index] == elementId.salt && state[elementId.index] == 1;
		}

		Elements elements;
		States state;
		std::vector<SaltT> salts;
		std::stack<IdxT> emptySlots;
	};



}
