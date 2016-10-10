#pragma once
#include "value.h"

namespace json {

///Allows to break immutability of json structure in some special situations
/**
 * The class implements and controls special _proxy_ object which can change the target over time
 * without need to rebuild the whole tree. Note because it breaks immutability and also
 * breaks loopless ability of the json-structure, it strongly unadvised to use this class
 * in common cases. The feature should be used only in very specific and rare situations.
 *
 * The class MutableLink has many limitations as well. Allowing loops in the json-structure also
 * breaks garbage collecting through the reference counters. To prevent this posibility, the MutableLink
 * is not part of reference counting. This means, that instance of the MutableLink must be
 * allocated separately and controlled by outside. The instance acts as weak reference, once
 * it is destroyed, it is interpreted as undefined
 *
 * The MutableLink can be presented also as some kind of a gateway, which is open as long as the
 * instance exists. The gateway can be added as a value to any place in the json-structure. The
 * gateway can be set to point to any other existing json-value and this can be changed
 * without need to recreate json-structure above.
 */
class MutableLink {

public:

	///Constructs mutable link which initially points to null
	MutableLink();
	///Constructs mutable link which initially points to the specified target
	/**
	 * @param target have to be mapped to the interface of the link
	 */
	MutableLink(const Value &target);
	///Destroys the mutable link
	/** Function sets link to undefined */
	~MutableLink();


	///Retrieves link object
	/** The value maps current target and can be put anywhere the value is expected */
	Value getLink() const ;


	///Changes target of the linl
	/** Changing the target is the act which is breaking immutability and loopless. Function
	 * maps the value to the link causing that internal value is changed everywhere where the
	 * link is put.
	 *
	 * @param target new target
	 *
	 * @note Multithreading note: By changing value through the link doesn't affect threads
	 * which already working with the old value.
	 */
	void setTarget(const Value &target);

	MutableLink(const MutableLink &) = delete;
	MutableLink& operator=(const MutableLink &) = delete;

protected:

	class LinkObject;
	typedef RefCntPtr<LinkObject> PLinkObject;

	PLinkObject link;


};



}
