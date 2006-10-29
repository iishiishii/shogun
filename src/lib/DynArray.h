/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2006 Soeren Sonnenburg
 * Copyright (C) 1999-2006 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_

#define DYNARRAY_STATISTICS

#include "lib/common.h"
#include "lib/Mathematics.h"

/** dynamic array, i.e. array that can be used like a list or an array.
it grows and shrinks dynamically, while elements can be accessed via index
performance tuned for simple types as float etc.
for hi-level objects only store pointers
*/
template <class T> class CDynamicArray;

template <class T> class CDynamicArray
{
public:
CDynamicArray(INT resize_granularity = 128, INT p_last_element_idx=-1)
	: free_array(true) 
#ifdef DYNARRAY_STATISTICS
		, name(NULL), stat_const_element(0), stat_element(0), stat_set_element(0), stat_get_element(0), stat_operator(0), stat_const_operator(0), stat_set_array(0), stat_get_array(0), stat_resize_array(0)
#endif
	{
		this->resize_granularity = resize_granularity;
		
		array = (T*) calloc(resize_granularity, sizeof(T));
		ASSERT(array);
		
		num_elements = resize_granularity;
		last_element_idx = p_last_element_idx;
	}
	
CDynamicArray(T* p_array, INT p_num_elements, INT p_array_size, bool p_free_array=true, bool p_copy_array=false)
	: array(NULL), free_array(false)
#ifdef DYNARRAY_STATISTICS
		, name(NULL), stat_const_element(0), stat_element(0), stat_set_element(0), stat_get_element(0), stat_operator(0), stat_const_operator(0), stat_set_array(0), stat_get_array(0), stat_resize_array(0)
#endif
	{
		set_array(p_array, p_num_elements, p_array_size, p_free_array, p_copy_array) ;
	}
	
CDynamicArray(const T* p_array, INT p_num_elements, INT p_array_size)
	: array(NULL), free_array(false)
#ifdef DYNARRAY_STATISTICS
		, name(NULL), stat_const_element(0), stat_element(0), stat_set_element(0), stat_get_element(0), stat_operator(0), stat_const_operator(0), stat_set_array(0), stat_get_array(0), stat_resize_array(0)
#endif
	{
		set_array(p_array, p_num_elements, p_array_size) ;
	}
	
	~CDynamicArray()
	{
#ifdef DYNARRAY_STATISTICS
		if (!name)
			name="unnamed" ;
		CIO::message(M_DEBUG, "destroying CDynamicArray array '%s' of size %i(%i)\n", name, num_elements, last_element_idx) ;
		CIO::message(M_DEBUG, "access statistics:\nconst element    %i\nelement    %i\nset_element    %i\nget_element    %i\nstat_operator[]    %i\nconst_operator[]    %i\nset_array    %i\nget_array    %i\nresize_array    %i\n", stat_const_element, stat_element, stat_set_element, stat_get_element, stat_operator, stat_const_operator, stat_set_array, stat_get_array, stat_resize_array) ;
#endif
		if (free_array)
			free(array);
	}

#ifdef DYNARRAY_STATISTICS
	inline void set_name(const char * p_name) 
	{
		name = p_name ;
	}
#endif

	/// set the resize granularity and return what has been set (minimum is 128) 
	inline INT set_granularity(INT g)
	{
		g=CMath::max(g,128);
		this->resize_granularity = g;
		return g;
	}

	/// return total array size (including granularity buffer)
	inline INT get_array_size()
	{
		return num_elements;
	}

	/// return total array size (including granularity buffer)
	inline INT get_dim1()
	{
		return num_elements;
	}
	
	/// return index of element which is at the end of the array
	inline INT get_num_elements()
	{
		return last_element_idx+1;
	}
	
	///return array element at index
	inline const T& get_element(INT index) const
	{
		ASSERT((array != NULL) && (index >= 0) && (index <= last_element_idx));
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_get_element++ ;
#endif
		return array[index];
	}
	
	///set array element at index 'index' return false in case of trouble
	inline bool set_element(const T& element, INT index)
	{
		ASSERT((array != NULL) && (index >= 0));
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_set_element++ ;
#endif
		if (index <= last_element_idx)
		{
			array[index]=element;
			return true;
		}
		else if (index < num_elements)
		{
			//CIO::message(M_DEBUG, "changing last_element idx  %i to %i elements in set_element\n", last_element_idx, index) ;
			array[index]=element;
			last_element_idx=index;
			return true;
		}
		else
		{
			//CIO::message(M_DEBUG, "resizing array from %i to %i elements in set_element\n", num_elements, index) ;
			if (resize_array(index))
				return set_element(element, index);
			else
				return false;
		}
	}
	
	inline const T& element(INT idx1) const
	{
#ifndef DYNARRAY_STATISTICS
		// hack to get rid of the const
		((CDynamicArray<T>*)this)->stat_const_element++ ;
#endif
		return get_element(idx1) ;
	}

	inline T& element(INT index) 
	{
		ASSERT((array != NULL) && (index >= 0));
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_element++ ;
#endif
		if (index <= last_element_idx)
		{
			return array[index] ;
		}
		else if (index < num_elements)
		{
			//CIO::message(M_DEBUG, "changing last_element idx  %i to %i elements in element\n", last_element_idx, index) ;
			last_element_idx=index;
			return array[index] ;
		}
		else
		{
			//CIO::message(M_DEBUG, "resizing array from %i to %i elements in element\n", num_elements, index) ;
			resize_array(index) ;
			return element(index);
		}
	}

	///set array element at index 'index' return false in case of trouble
	inline bool insert_element(const T& element, INT index)
	{
		if (append_element(get_element(last_element_idx)))
		{
			for (INT i=last_element_idx-1; i>index; i--)
			{
				array[i]=array[i-1];
			}
			array[index]=element;
		}
		else
			return false;
	}

	///set array element at index 'index' return false in case of trouble
	inline bool append_element(const T& element)
	{
		return append_element(element, last_element_idx+1);
	}

	///delete array element at idx (does not call delete[] or the like)
	inline bool delete_element(INT idx)
	{
		if (idx>=0 && idx<=last_element_idx)
		{
			for (INT i=idx; i<last_element_idx; i++)
				array[i]=array[i+1];

			array[last_element_idx]=0;
			last_element_idx--;

			if ( num_elements - last_element_idx >= resize_granularity)
				resize_array(last_element_idx);
		}
		else
			return false;
	}
	
	///resize the array 
	bool resize_array(INT n)
	{
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_resize_array++ ;
#endif
		INT new_num_elements= ((n/resize_granularity)+1)*resize_granularity;

		T* p= (T*) realloc(array, sizeof(T)*new_num_elements);
		if (p)
		{
			array=p;
			if (new_num_elements > num_elements)
				memset(&array[num_elements], 0, (new_num_elements-num_elements)*sizeof(T));
			else if (n+1<new_num_elements)
					memset(&array[n+1], 0, (new_num_elements-n-1)*sizeof(T));

			//in case of shrinking we must adjust last element idx
			if (n-1<last_element_idx)
				last_element_idx=n-1;

			num_elements=new_num_elements;
			return true;
		}
		else
			return false;
	}

	/// get the array
	/// call get_array just before messing with it DO NOT call any [],resize/delete functions after get_array(), the pointer may become invalid !
	inline T* get_array()
	{
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_get_array++ ;
#endif
		return array;
	}

	/// set the array pointer and free previously allocated memory
	inline void set_array(T* array, INT num_elements, INT array_size, bool free_array=true, bool copy_array=false)
	{
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_set_array++ ;
#endif
		if (this->free_array)
			free(this->array);
		if (copy_array)
		{
			this->array=(T*)malloc(array_size*sizeof(T)) ;
			memcpy(this->array, array, array_size*sizeof(T)) ;
		}
		else
			this->array=array;
		this->num_elements=array_size;
		this->last_element_idx=num_elements-1;
		this->free_array=free_array ;
	}

	/// set the array pointer and free previously allocated memory
	inline void set_array(const T* array, INT num_elements, INT array_size)
	{
#ifdef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_set_array++ ;
#endif
		if (this->free_array)
			free(this->array);
		this->array=(T*)malloc(array_size*sizeof(T)) ;
		memcpy(this->array, array, array_size*sizeof(T)) ;
		this->num_elements=array_size;
		this->last_element_idx=num_elements-1;
		this->free_array=free_array ;
	}

	/// clear the array (with zeros)
	inline void clear_array()
	{
		if (last_element_idx >= 0)
			memset(array, 0, (last_element_idx+1)*sizeof(T));
	}


	/// operator overload for array read only access
	/// use set_element() for write access (will also make the array dynamically grow)
	///
	/// DOES NOT DO ANY BOUNDS CHECKING
	inline const T& operator[](INT index) const
	{
#ifndef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_const_operator++ ;
#endif
		return array[index];
	}
	
	inline T& operator[](INT index) 
	{
#ifndef DYNARRAY_STATISTICS
		((CDynamicArray<T>*)this)->stat_operator++ ;
#endif
		return element(index);
	}
	
	///// operator overload for array assignment
	CDynamicArray<T>& operator=(CDynamicArray<T>& orig)
	{
		resize_granularity=orig.resize_granularity;
		memcpy(array, orig.array, sizeof(T)*orig.num_elements);
		num_elements=orig.num_elements;
		last_element_idx=orig.last_element_idx;

		return *this;
	}

protected:
	/// shrink/grow step size
	INT resize_granularity;

	/// memory for dynamic array
	T* array;

	/// the number of potentially used elements in array
	INT num_elements;

	/// the element in the array that has largest index
	INT last_element_idx;

	/// 
	bool free_array ;

#ifdef DYNARRAY_STATISTICS
	const char * name ;
	INT stat_const_element, stat_element, stat_set_element, stat_get_element, stat_operator, stat_const_operator, stat_set_array, stat_get_array, stat_resize_array;
#endif
};
#endif
