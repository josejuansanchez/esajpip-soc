#ifndef _JPEG2000_RANGE_H_
#define _JPEG2000_RANGE_H_


#include <iostream>
#include <assert.h>
#include <limits.h>


namespace jpeg2000
{
  using namespace std;


  /**
   * Represents a range of integer values, defined by
   * two values, first and last, which are assumed to
   * be included in the range. Some basic operations
   * are defined for easing the work with ranges.
   */
  class Range
  {
  public:
    int first;		///< First value of the range
    int last;		///< Last value of the range
    int max_val;	///< Maximum range value


    /**
     * Initializes the object.
     */
    Range()
    {
      max_val = INT_MAX;
      first = 0;
      last = 0;
    }

    /**
     * Initializes the object.
     * @param first First value.
     * @param last Last value.
     */
    Range(int first, int last, int max_val = INT_MAX)
    {
      //assert((first >= 0) && (first <= last));
      this->max_val = max_val;
      this->first = first;
      this->last = last;
    }

    /**
     * Copy constructor.
     */
    Range(const Range& range)
    {
      *this = range;
    }

    /**
     * Copy assignment.
     */
    Range& operator=(const Range& range)
    {
      max_val = range.max_val;
      first = range.first;
      last = range.last;

      return *this;
    }

    /**
     * Returns <code>true</code> if the first value if
     * greater or equal to zero, and it is less or
     * equal to the last value.
     */
    bool IsValid() const
    {
      return ((first >= 0) && (first < max_val) &&
      				(last >= 0) && (last < max_val));
    }

    /**
     * Returns an item of the range, starting at the
     * first value.
     * @param i Item index.
     * @return first + i.
     */
    int GetItem(int i) const
    {
      //return (first + i);
      return ((first + i) % max_val);
    }

    /**
     * Returns the index of an item of the range.
     * @param item Item of the range.
     * @return item - first.
     */
    int GetIndex(int item) const
    {
        //return (item - first);
    	int res = (item - first);
    	return ((res >= 0) ? res : (item + (max_val - first)));
    }

    bool GetNext(int& val) const
    {
    	if(val != last) {
    		val = (val + 1) % max_val;
    		return false;

    	} else {
    		val = first;
    		return true;
    	}
    }

    /**
     * Returns the length of the range (last - first + 1).
     */
    int Length() const
    {
      //return (last - first + 1);
      return ((first <= last) ? (last - first + 1) : (max_val - first + last + 1));
    }

    friend bool operator==(const Range& a, const Range& b)
    {
       return ((a.first == b.first) && (a.last == b.last) && (a.max_val == b.max_val));
    }

    friend bool operator!=(const Range& a, const Range& b)
    {
      return ((a.first != b.first) || (a.last != b.last) || (a.max_val != b.max_val));
    }

    friend ostream& operator << (ostream &out, const Range &range)
    {
      if(range.IsValid()) out << "[" << range.first << " - " << range.last << "]";
      else out << "[ ]";

      return out;
    }

    virtual ~Range()
    {
    }
  };

}


#endif /* _JPEG2000_RANGE_H_ */
