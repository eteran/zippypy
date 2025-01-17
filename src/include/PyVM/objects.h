// Copyright 2015 by Intigua, Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "defs.h"
#include "except.h"
#include "ObjPool.h"
#include "PyVM.h"
#include "utils.h"


#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>

struct TupleObject;

inline bool isStrType(Object::Type t) {
	return t == Object::STR || t == Object::USTR;
}

#define CHECK_TYPE(r, T) CHECK(r->type == Object::typeValue<T>(), "wrong type cast expected " << Object::typeName<T>() << ", got " << r->typeName())

// these should be preferred over costy dynamic_cast
template <typename T> // an Object type
PoolPtr<T> checked_cast(ObjRef &r) {
	CHECK_TYPE(r, T);
	return static_pcast<T>(r);
}

template <typename T> // an Object type
const PoolPtr<T> checked_cast(const ObjRef &r) {
	CHECK_TYPE(r, T);
	return static_pcast<T>(r);
}

template <typename T> // an Object type
const T *checked_cast(Object *r) {
	CHECK_TYPE(r, T);
	return static_cast<T *>(r);
}

template <typename T>
PoolPtr<T> checked_dynamic_pcast(const ObjRef &o) {
	T *p = dynamic_cast<T *>(o.get());
	CHECK(p != nullptr, "bad dynamic cast");
	return PoolPtr<T>(p);
}

template <typename T>
const T *checked_dynamic_pcast(const Object *o) {
	const T *p = dynamic_cast<const T *>(o);
	CHECK(p != nullptr, "bad dynamic cast");
	return p;
}

// exception raised in python code
class PyRaisedException : public PyException {
public:
	PyRaisedException(const ObjRef &_inst, const ObjRef &_b, const ObjRef &_c)
		: inst(_inst), b(_b), c(_c), PyException("Thrown from script: " + stdstr(_inst)) {}
	ObjRef inst, b, c;
};

struct BoolObject : public Object {
	BoolObject(bool _v)
		: Object(BOOL), v(_v) {}
	bool v;
};

struct IntObject : public Object {
	IntObject(uint _v)
		: Object(INT), v(_v) {}
	IntObject(int _v)
		: Object(INT), v(_v) {}
	IntObject(int64_t _v)
		: Object(INT), v(_v) {}
	IntObject(uint64_t _v)
		: Object(INT) { *(uint64_t *)&v = _v; } // copy the binary value
	int64_t v;
};

struct FloatObject : public Object {
	FloatObject(double _v)
		: Object(FLOAT), v(_v) {}
	double v;
};

struct ISubscriptable {
	virtual ~ISubscriptable() = default;
	virtual void   setSubscr(const ObjRef &key, const ObjRef &value) = 0;
	virtual ObjRef getSubscr(const ObjRef &key, PyVM *vm)            = 0;

protected:
	int extractIndex(const ObjRef &key, size_t size);
};

struct IIterator {
	virtual ~IIterator() = default;
	virtual bool next(ObjRef &obj) = 0;
};

struct IteratorObject : public Object, IIterator {
	IteratorObject()
		: Object(ITERATOR) {}
	// if there is a next object, set obj to it and return true. if we're at the end, return false.
};
using IterObjRef = PoolPtr<IteratorObject>;

struct IIterable {
	virtual ~IIterable() = default;
	// _vm needed for creating a new object
	// returns an object that implements IIterator
	virtual ObjRef iter(PyVM *_vm) = 0;
};

// an iterator over an object. the object needs to have a member 'v' which is an c++ sequence
template <typename T>
struct GenericIterObject : public IteratorObject, public IIterable {
	GenericIterObject(const PoolPtr<T> &l, PyVM *vm)
		: of(l), m_vm(vm) {}
	void clear() override {
		of.reset();
	}
	bool next(ObjRef &obj) override {
		if (i >= (int)of->v.size())
			return false;
		obj = m_vm->makeFromT(of->v[i++]);
		return true;
	}
	ObjRef iter(PyVM *_vm) override { // an iterator is also iterable which returns itself
		(void)_vm;
		return ObjRef(this);
	}

	int        i = 0;
	PoolPtr<T> of;
	PyVM *     m_vm;
};

// T is the this type: ListObject, StrObject etc'
template <typename T>
struct GenericIterable : public IIterable {
	// _vm needed for creating a new object
	ObjRef iter(PyVM *_vm) override {
		return _vm->alloc(new GenericIterObject<T>(PoolPtr<T>(static_cast<T *>(this)), _vm));
	}
};

template <typename T>
struct MapIterObject : public IteratorObject, public IIterable {
	MapIterObject(const PoolPtr<T> &l, PyVM *vm)
		: of(l), m_vm(vm) {
		i = of->v.begin();
	}

	void clear() override {
		of.reset();
	}

	bool next(ObjRef &obj) override {
		if (i == of->v.end())
			return false;
		obj = m_vm->makeFromT((i++)->first);
		return true;
	}

	ObjRef iter(PyVM *_vm) override { // an iterator is also iterable which returns itself
		(void)_vm;
		return ObjRef(this);
	}

	PoolPtr<T> of;
	using MapType = decltype(of->v);
	typename MapType::iterator i;

	PyVM *m_vm;
};

// T is the this type: ListObject, StrObject etc'
template <typename T>
struct MapIterable : public IIterable {
	// _vm needed for creating a new object
	ObjRef iter(PyVM *_vm) override {
		return _vm->alloc(new MapIterObject<T>(PoolPtr<T>(static_cast<T *>(this)), _vm));
	}
};

class SliceObject : public Object {
public:
	SliceObject(bool h_start, int _start, bool h_stop, int _stop, bool h_step, int _step)
		: Object(SLICE), has_start(h_start), start(_start), has_stop(h_stop), stop(_stop), has_step(h_step), step(_step) {}

	template <typename T>
	T slice_step(const T &o);

	int  start, stop, step;
	bool has_start, has_stop, has_step;
};

using SliceObjRef = PoolPtr<SliceObject>;

// inspired by CPython PySlice_GetIndicesEx() in sliceobject.c
template <typename T>
T SliceObject::slice_step(const T &o) {
	if (!has_step)
		step = 1;
	int length = (int)o.size();
	if (!has_start)
		start = step < 0 ? length : 0;
	else {
		if (start < 0) start += length;
		if (start < 0) start = (step < 0) ? -1 : 0;
		if (start >= length)
			start = (step < 0) ? length - 1 : length;
	}
	if (!has_stop)
		stop = step < 0 ? -1 : length;
	else {
		if (stop < 0) stop += length;
		if (stop < 0) stop = (step < 0) ? -1 : 0;
		if (stop >= length)
			stop = (step < 0) ? length - 1 : length;
	}

	T   res;
	int slicelength;
	if ((step < 0 && stop >= start) || (step > 0 && start >= stop))
		return res; // len = 0
	else if (step < 0)
		slicelength = (stop - start + 1) / step + 1;
	else
		slicelength = (stop - start - 1) / step + 1;
	res.resize(slicelength);
	for (int cur = start, i = 0; i < slicelength; cur += step, ++i) {
		res[i] = o[cur];
	}
	return res;
}

// T is the this type, ElemT is the type of the elements: ObjRef or char
//  we can't get the type of 'v' so we need to get this as an additional template arg. when VC supports typeof() this will be possible
template <typename T, typename ElemT>
struct GenericSubscriptable : public ISubscriptable {
	ObjRef getSubscr(const ObjRef &key, PyVM *vm) override {
		auto &tv = static_cast<T *>(this)->v;
		if (key->type == Object::SLICE) {
			return vm->makeFromT(static_pcast<SliceObject>(key)->slice_step(tv));
		}
		return vm->makeFromT(tv[extractIndex(key, tv.size())]);
	}
	void setSubscr(const ObjRef &key, const ObjRef &value) override;
};

struct StrBaseObject : public Object {
	StrBaseObject(Object::Type t)
		: Object(t) {}
	virtual int    at(int i) const = 0;
	virtual int    size() const    = 0;
	virtual size_t ptr() const     = 0;
};

enum StrModifier {
	STRMOD_NONE,  // get the string as it is
	STRMOD_CASEI, // get lower-case versin of the string
	STRMOD_PATH   // path normalized version of the string (only on windows)
};

// various caches for an StrObject (ansi)
struct StrExtension {
	StrExtension(const std::string *_v)
		: v(_v), has_iv(false), has_pathv(false), has_wv(false), has_wiv(false), has_wpathv(false) {}
	bool               has_iv, has_pathv, has_wv, has_wiv, has_wpathv;
	const std::string *v;
	std::string        iv;     // lower case ansi
	std::string        pathv;  // path-normalized ansi
	std::wstring       wv;     // wide-char cache
	std::wstring       wiv;    // lower case wide-char
	std::wstring       wpathv; // path-normalized wide-char

	std::string *getStr(StrModifier mod) {
		if (mod == STRMOD_CASEI) {
			if (!has_iv) {
				iv     = toLower(*v);
				has_iv = true;
			}
			return &iv;
		}
		/* TBD     if (mod == STRMOD_PATH) { // active only in windows see PATH_MODIFIER_
            if (!has_pathv) {
                pathv = os::FSUtils::pathNormalized(*v, os::FSUtils::LOW_CASE);
                has_pathv = true;
            }
            return &pathv;
        }*/
		THROW("Unexpected string (StrE) modifier" << mod);
	}
	std::wstring *getWStr(StrModifier mod) {
		if (!has_wv) { // need to make wv anyway
			wv     = wstrFromAnsi(*v);
			has_wv = true;
		}
		if (mod == STRMOD_NONE) {
			return &wv;
		}
		if (mod == STRMOD_CASEI) {
			if (!has_wiv) {
				wiv     = toLower(wv);
				has_wiv = true;
			}
			return &wiv;
		}
		/*   TBD     if (mod == STRMOD_PATH) { // active only in windows see PATH_MODIFIER_
            if (!has_wpathv) {
                wpathv = os::FSUtils::pathNormalized(wv, os::FSUtils::LOW_CASE);
                has_wpathv = true;
            }
            return &wpathv;
        }*/
		THROW("Unexpected std::wstring (StrE) modifier" << mod);
	}
};

struct StrObject : public StrBaseObject, public GenericIterable<StrObject>, public GenericSubscriptable<StrObject, char> {
	StrObject()
		: StrBaseObject(STR) {}

	explicit StrObject(char c)
		: StrBaseObject(STR), v(1, c) {}

	explicit StrObject(const std::string &_v)
		: StrBaseObject(STR), v(_v) {}

	int size() const override {
		return (int)v.size();
	}

	int at(int i) const override {
		return (int)(uchar)v[i]; // return value between 0-255 like what ord() does
	}

	size_t ptr() const override {
		return (size_t)v.data();
	}

	const std::string *getStr(StrModifier mod) {
		if (mod == STRMOD_NONE)
			return &v;
		if (ext.get() == nullptr)
			ext.reset(new StrExtension(&v));
		return ext->getStr(mod);
	}

	const std::wstring *getWStr(StrModifier mod) {
		// STRs are immutable so it's ok to cache it once
		if (ext.get() == nullptr)
			ext.reset(new StrExtension(&v));
		return ext->getWStr(mod);
	}

	std::string                   v;
	std::unique_ptr<StrExtension> ext;
};
using StrObjRef = PoolPtr<StrObject>;

// various caches for an StrObject (ansi)
struct UnicodeExtension {
	UnicodeExtension(const std::wstring *_wv)
		: wv(_wv) {}

	bool has_wiv = false;
	bool has_wpathv = false;
	const std::wstring *wv;
	std::wstring        wiv;    // lower case wide-char
	std::wstring        wpathv; // path-normalized wide-char

	std::wstring *getWStr(StrModifier mod) {
		if (mod == STRMOD_CASEI) {
			if (!has_wiv) {
				wiv     = toLower(*wv);
				has_wiv = true;
			}
			return &wiv;
		}
		/*     if (mod == STRMOD_PATH) {
            if (!has_wpathv) {
                wpathv = os::FSUtils::pathNormalized(*wv, os::FSUtils::LOW_CASE);
                has_wpathv = true;
            }
            return &wpathv;
        }*/
		THROW("Unexpected std::wstring (UnicodeE) modifier" << mod);
	}
};

enum EEncoding {
	ENC_UTF8  = 0,
	ENC_ASCII = 1
};
struct UnicodeObject : public StrBaseObject, public GenericIterable<UnicodeObject>, public GenericSubscriptable<UnicodeObject, wchar_t> {
	UnicodeObject()
		: StrBaseObject(USTR) {}
	explicit UnicodeObject(wchar_t c)
		: StrBaseObject(USTR), v(1, c) {}
	explicit UnicodeObject(const std::wstring &_v)
		: StrBaseObject(USTR), v(_v) {}
	UnicodeObject(const std::string &_v, EEncoding enc)
		: StrBaseObject(USTR) {
		if (enc == ENC_UTF8)
			CHECK(wstrFromUtf8(_v, &v), "Failed utf-8 to std::wstring conversion");
		else if (enc == ENC_ASCII)
			v = wstrFromAnsi(_v);
	}
	explicit UnicodeObject(const StrObjRef &s)
		: StrBaseObject(USTR) {
		v = wstrFromAnsi(s->v);
	}

	static ObjRef fromStr(const ObjRef &s, PyVM *vm);

	int size() const override {
		return (int)v.size();
	}

	int at(int i) const override {
		return (int)(unsigned short)v[i];
	}

	size_t ptr() const override {
		return (size_t)v.data();
	}

	const std::wstring *getWStr(StrModifier mod) {
		if (mod == STRMOD_NONE)
			return &v;
		if (ext.get() == nullptr)
			ext.reset(new UnicodeExtension(&v));
		return ext->getWStr(mod);
	}

	std::wstring                      v;
	std::unique_ptr<UnicodeExtension> ext;
};

struct ListObject : public Object, public GenericSubscriptable<ListObject, ObjRef>, public GenericIterable<ListObject> {
	ListObject(const std::vector<ObjRef> &o)
		: Object(LIST), v(o) {}
	ListObject(Type _type = LIST)
		: Object(_type) {}
	int size() const {
		return (int)v.size();
	}
	void clear() override {
		v.clear(); // vector clear
	}

	void append(const ObjRef &a) {
		v.push_back(a);
	}
	void prepend(const ObjRef &a) {
		v.insert(v.begin(), a);
	}

	ObjRef pop(int index) {
		ObjRef ref = v.at(index);
		v.erase(v.begin() + index);
		return ref;
	}

	std::vector<ObjRef> v;
};
using ListObjRef = PoolPtr<ListObject>;

struct TupleObject : public ListObject {
	TupleObject()
		: ListObject(TUPLE) {}
};

using TupleObjRef = PoolPtr<TupleObject>;

template <typename T>
struct MapKeyValueIterObject : public MapIterObject<T> {
	MapKeyValueIterObject(const PoolPtr<T> &l, PyVM *vm)
		: MapIterObject<T>(l, vm) {}

	bool next(ObjRef &obj) override {
		if (this->i == this->of->v.end())
			return false;
		auto t = this->m_vm->alloct(new TupleObject);
		t->append(this->m_vm->makeFromT(this->i->first));
		t->append(this->i->second);
		obj = ObjRef(t);
		++this->i;
		return true;
	}
};

//BOOST_STATIC_ASSERT(sizeof(int16) == sizeof(wchar_t));

template <typename PT>
struct PObjType;
template <>
struct PObjType<short int> { using ot = IntObject; };
template <>
struct PObjType<int> { using ot = IntObject; };
template <>
struct PObjType<uint> { using ot = IntObject; };
template <>
struct PObjType<int64_t> { using ot = IntObject; };
template <>
struct PObjType<uint64_t> { using ot = IntObject; };
template <>
struct PObjType<bool> { using ot = BoolObject; };
template <>
struct PObjType<std::string> { using ot = StrObject; };
template <>
struct PObjType<std::string &> { using ot = StrObject; };
template <>
struct PObjType<const std::string &> { using ot = StrObject; };
template <>
struct PObjType<const char *> { using ot = StrObject; };
template <>
struct PObjType<char> { using ot = StrObject; }; // single character -> string
template <>
struct PObjType<std::wstring> { using ot = UnicodeObject; };
template <>
struct PObjType<std::wstring &> { using ot = UnicodeObject; };
template <>
struct PObjType<const std::wstring &> { using ot = UnicodeObject; };
template <>
struct PObjType<const wchar_t *> { using ot = UnicodeObject; };
template <>
struct PObjType<wchar_t> { using ot = UnicodeObject; };
template <>
struct PObjType<double> { using ot = FloatObject; };
template <>
struct PObjType<float> { using ot = FloatObject; };

template <typename ET>
struct PObjType<std::vector<ET>> { using ot = ListObject; };

// returns the type of python object that corresponds to the given C type.
// to be used as template argument
#define POBJ_TYPE(t) typename PObjType<t>::ot
#define PSTROBJ_TYPE(tc) typename PObjType<std::basic_string<tc>>::ot

template <typename T>
inline void Object::checkTypeT() {
	checkType(Object::typeValue<POBJ_TYPE(T)>());
}

// these are needed instead of extract<> since template instantiation of function template does not take the template argument
template <typename T>
struct Extract {
	T operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		o->checkTypeT<T>();
		return (T) static_cast<POBJ_TYPE(T) *>(o.get())->v;
	}
};
// some special cases are below
template <>
struct Extract<uint64_t> {
	int64_t operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		o->checkType(Object::INT);
		return *(uint64_t *)&static_cast<IntObject *>(o.get())->v;
	}
};
template <>
struct Extract<char> {
	char operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		if (o->type == Object::STR) {
			auto *s = static_cast<StrObject *>(o.get());
			CHECK(s->size() == 1, "string needs to be single character");
			return s->v[0];
		}
		if (o->type == Object::INT) {
			auto *s = static_cast<IntObject *>(o.get());
			return *(char *)&(s->v); // doesn't handle overflow
		}
		THROW("Extract<char> unexpectyed type:" << o->typeName());
	}
};
template <>
struct Extract<wchar_t> {
	wchar_t operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		if (o->type == Object::USTR) {
			auto *s = static_cast<UnicodeObject *>(o.get());
			CHECK(s->size() == 1, "ustring needs to be single character");
			return s->v[0];
		}
		if (o->type == Object::INT) {
			auto *s = static_cast<IntObject *>(o.get());
			return *(wchar_t *)&(s->v); // doesn't handle overflow
		}
		THROW("Extract<wchar_t> unexpectyed type:" << o->typeName());
	}
};

template <>
struct Extract<double> {
	double operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		if (o->type == Object::INT) {
			return (double)static_cast<IntObject *>(o.get())->v;
		}
		if (o->type == Object::FLOAT) {
			return static_cast<FloatObject *>(o.get())->v;
		}
		THROW("Extract<double> unexpectyed type:" << o->typeName());
	}
};
template <typename ET> // partial specialization for a uniform list of anything
struct Extract<std::vector<ET>> {
	std::vector<ET> operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		auto            lo = dynamic_pcast<ListObject>(o);
		std::vector<ET> v;
		for (auto it = lo->v.begin(); it != lo->v.end(); ++it) {
			v.push_back(Extract<ET>()(*it));
		}
		return v;
	}
};

// get a string pointer according to TC from an STR or USTR object
template <typename TC>
const std::basic_string<TC> *extractStrPtr(const ObjRef &o, StrModifier mod);

template <>
inline const std::string *extractStrPtr(const ObjRef &o, StrModifier mod) {
	return checked_cast<StrObject>(o)->getStr(mod);
}
template <>
inline const std::wstring *extractStrPtr(const ObjRef &o, StrModifier mod) {
	CHECK(!o.isNull(), "Extract from nullptr ref");
	if (o->type == Object::USTR) {
		return static_cast<UnicodeObject *>(o.get())->getWStr(mod);
	}
	if (o->type == Object::STR) {
		return static_cast<StrObject *>(o.get())->getWStr(mod);
	}
	THROW("Extract<double> unexpectyed type:" << o->typeName());
}

// normal Extract using the above, to extract wstr to an str
template <>
struct Extract<std::wstring> {
	std::wstring operator()(const ObjRef &o) {
		return *extractStrPtr<wchar_t>(o, STRMOD_NONE);
	}
};

template <typename T>
struct Extract<const T &> { // convertor for const reference of something
	T operator()(const ObjRef &o) {
		return Extract<T>()(o);
	}
};
template <>
struct Extract<ObjRef> {
	ObjRef operator()(const ObjRef &o) {
		return o;
	}
};

// needs to be declared after extract()
template <typename T, typename ElemT>
void GenericSubscriptable<T, ElemT>::setSubscr(const ObjRef &key, const ObjRef &value) {
	auto &tv                         = static_cast<T *>(this)->v;
	tv[extractIndex(key, tv.size())] = Extract<ElemT>()(value);
}

// --------------------------------------------------- complex types --------------------------------------------------

int64_t hashNum(const ObjRef &argref);
bool    objEquals(const ObjRef &lhsref, const ObjRef &rhsref, PyVM *vm);

struct ObjEquals {
	PyVM *vm;
	bool  operator()(const ObjRef &lhsref, const ObjRef &rhsref) const {
        return objEquals(lhsref, rhsref, vm);
	}
};

// WARNNING!!!!!!!!!!!!!!!!!!!!! not a complete implementation, does not handle collisions
struct DictObject : public Object, public ISubscriptable, public MapIterable<DictObject> {
	// vm is needed for the compare which needs it in order to create unicode objects if the need of conversion arises
	DictObject(PyVM *vm)
		: Object(DICT), v(4, hashNum, ObjEquals{vm}) {}

	void clear() override {
		v.clear(); // map clear
	}

	void setSubscr(const ObjRef &key, const ObjRef &value) override {
		v[key] = value;
	}

	ObjRef getSubscr(const ObjRef &key, PyVM *) override {
		return v[key];
	}

	virtual ObjRef pop(const ObjRef &key) {
		ObjRef ref = v[key];
		v.erase(key);
		return ref;
	}

	std::unordered_map<ObjRef, ObjRef, decltype(hashNum) *, ObjEquals> v;
};

using DictObjRef = PoolPtr<DictObject>;

struct StrDictObject : public Object, public ISubscriptable, public MapIterable<StrDictObject> {
	StrDictObject(const NameDict &nd)
		: Object(STRDICT), v(nd) {}
	StrDictObject()
		: Object(STRDICT) {}

	void clear() override {
		v.clear(); // map clear
	}

	void setSubscr(const ObjRef &key, const ObjRef &value) override {
		const std::string &sk = checked_cast<StrObject>(key)->v;
		v[sk]                 = value;
	}

	ObjRef getSubscr(const ObjRef &key, PyVM *) override {
		const std::string &sk = checked_cast<StrObject>(key)->v;
		return v[sk];
	}

	virtual ObjRef pop(const ObjRef &key) {
		std::string strKey = Extract<std::string>()(key);
		ObjRef      ref    = v[strKey];
		v.erase(strKey);
		return ref;
	}

	NameDict v;
};

using StrDictObjRef = PoolPtr<StrDictObject>;

template <typename F>
void forNameDict(const NameDict &dict, Object::Type t, F func) {
	for (auto it = dict.begin(); it != dict.end(); ++it) {
		if (it->second->type == t) {
			func(it->first, it->second);
		}
	}
}

// F takes C as argument
template <typename C, typename F>
void forNameDict(const NameDict &dict, F func) {
	for (auto it = dict.begin(); it != dict.end(); ++it) {
		if (it->second->type == Object::typeValue<C>()) {
			auto c = static_pcast<C>(it->second);
			func(it->first, c);
		}
	}
}

class CodeObject : public Object {
public:
	CodeObject()
		: Object(CODE) {}
	CodeObject(const CodeDefinition &co)
		: Object(CODE), m_co(co) {}
	int            lineFromIndex(int i) const;
	CodeDefinition m_co;
};

// values of co_flags. copied from python code.h
#define MCO_NEWLOCALS 0x0002 // means it has the local vars by index optimization (not used since it's not on for modules)
#define MCO_GENERATOR 0x0020
#define MCO_VARARGS 0x0004
#define MCO_VARKEYWORDS 0x0008

class Frame;
class ModuleObject;
using ModuleObjRef = PoolPtr<ModuleObject>;

struct CallableObject : public Object {
	CallableObject(Type _type, ModuleObjRef mod, uint propType = 0)
		: Object(_type, ICALLABLE | propType), m_module(mod), m_isStaticMethod(false) {}
	// from - the calling frame
	// frame - this call frame

	void clear() override {
		m_module.reset();
	}

	ModuleObjRef m_module; // module this object is defined in, for globals. may be nullptr if not relevant
	bool         m_isStaticMethod;
};

using CallableObjRef = PoolPtr<CallableObject>;

struct FuncObject : public CallableObject {
	FuncObject(const CodeObjRef &cobj, ModuleObjRef module)
		: CallableObject(FUNC, module), m_code(cobj) {}

	void clear() override {
		CallableObject::clear();
		m_code.reset();
	}

	ObjRef      call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override;

	std::string funcname() const override{
		return m_code->m_co.co_name;
	}

	CodeObjRef m_code;
};

using FuncObjRef = PoolPtr<FuncObject>;

// wrap a c function
struct ICWrap : public Object {
	ICWrap()
		: Object(CFUNC_WRAP) {}
	virtual int                argsCount()                      = 0;
	virtual ObjRef             call(CallArgs &)                 = 0;
	virtual const std::string &name()                           = 0;
	virtual void               setName(const std::string &name) = 0;
};

using ICWrapPtr = PoolPtr<ICWrap>;

struct CFuncObject : public CallableObject {
	CFuncObject(const ICWrapPtr &cwrap)
		: CallableObject(FUNC, ModuleObjRef()), wrap(cwrap) {}
	ObjRef      call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override;
	std::string funcname() const override {
		return wrap.isNull() ? std::string("<nullptr-cfunc>") : wrap->name();
	}
	void clear() override {
		CallableObject::clear();
		wrap.reset();
	}
	ICWrapPtr wrap;
};

struct ICtorWrap;

// wrapper for a C instance of a class
struct ICInstWrap : public Object {
	ICInstWrap()
		: Object(CINSTANCE_WRAP) {}

	void clear() override {
		m_ctor.reset();
	}

	~ICInstWrap() override = default;

	PoolPtr<ICtorWrap> m_ctor; // this needs to be a pointer and we don't want a shared_ptr
};
template <typename T>
struct CInstWrap : public ICInstWrap {
	virtual T *ptr() = 0;

	~CInstWrap() override = default;
};
// for pointer value given by the C user
template <typename T>
struct CInstWrapPtr : public CInstWrap<T> {
	CInstWrapPtr(T *_v = nullptr)
		: v(_v) {}

	T *ptr() override { return v; }
	T *        v;
};
// for by-value value given by the user
template <typename T, typename U> // store type T, return pointer of type U (a base type of U)
struct CInstWrapValue : public CInstWrap<U> {

	CInstWrapValue(const T &_v)
		: v(_v) {}

	U *ptr() override { return dynamic_cast<U *>(&v); }
	T          v;
};

template <typename T>
struct CInstWrapSharedPtr : public CInstWrap<T> {
	CInstWrapSharedPtr(std::shared_ptr<T> _v = nullptr)
		: v(_v) {}

	~CInstWrapSharedPtr() override = default;

	T *                ptr() override { return v.get(); }
	virtual std::shared_ptr<T> getSharedPtr() { return v; }

	std::shared_ptr<T> v;
};

// constructors calling

struct NoType {};
// used for defining ctor arguments
// more than one arguments not yet supported
template <typename A1 = NoType>
struct CtorDef {
	using t1 = A1; // used in class_<>()
};

struct ICtorWrap : public Object {
	ICtorWrap()
		: Object(CCTOR_WRAP) {}
	virtual PoolPtr<ICInstWrap> construct(PyVM *vm, CallArgs &args) = 0;
};
template <typename C, typename A1>
struct CtorWrap : public ICtorWrap {
	PoolPtr<ICInstWrap> construct(PyVM *vm, CallArgs &args) override {
		A1 a1 = Extract<A1>()(args[0]);
		return vm->alloct<ICInstWrap>(new CInstWrapSharedPtr<C>(std::make_shared<C>(vm, a1)));
	}
};

template <typename C> // partial specialization
struct CtorWrap<C, NoType> : public ICtorWrap {
	PoolPtr<ICInstWrap> construct(PyVM *vm, CallArgs &args) override {
		return vm->alloct<ICInstWrap>(new CInstWrapSharedPtr<C>(std::make_shared<C>(vm)));
	}
};

#include "cfunc.h"

class InstanceObject;
using InstanceObjRef = PoolPtr<InstanceObject>;

// represents a bound method
class MethodObject : public CallableObject {
public:
	MethodObject(const CallableObjRef &func, const InstanceObjRef &self)
		: CallableObject(METHOD, func->m_module), m_self(self), m_func(func) {}

	void clear() override {
		CallableObject::clear();
		m_self.reset();
		m_func.reset();
	}
	ObjRef      call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override;
	std::string funcname() const override ;

	// turn an unbounded method (in a class) to a bound method
	PoolPtr<MethodObject> bind(const InstanceObjRef &newself);

	InstanceObjRef m_self; // instance or None (creates circle)
	CallableObjRef m_func;
};

using MethodObjRef = PoolPtr<MethodObject>;

class ClassObject : public CallableObject //, public IAttrable
{
public:
	ClassObject(const std::string &name, ModuleObjRef module, PyVM *vm)
		: CallableObject(CLASS, module, IATTRABLE), m_name(name), m_vm(vm) {}
	// called from instruction BUILD_CLASS
	// called from create wrapper class for a C++ class
	ClassObject(const StrDictObjRef &methods, const std::vector<ObjRef> &bases, const std::string &name, ModuleObjRef module, PyVM *vm)
		: CallableObject(CLASS, module, IATTRABLE), m_dict(methods), m_name(name), m_vm(vm) {
		CHECK(bases.size() <= 1, "more that one base class not supported");
		if (bases.size() > 0)
			m_base = checked_cast<ClassObject>(bases[0]);
	}

	void clear() override {
		CallableObject::clear();
		m_dict.reset();
		m_base.reset();
		m_cwrap.reset();
	}

	// void makeMethods(const InstanceObjRef& i);
	ObjRef      call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override;
	std::string funcname() const override {
		return m_name;
	}

	ObjRef addMember(const ObjRef &o, const std::string &name) {
		m_dict->v[name] = o;
		return o;
	}

	template <typename F>
	ObjRef def(F func, const std::string &name) {
		auto ic = makeCWrap(func, m_vm);
		ic->setName(name);
		auto calb = static_pcast<CallableObject>(m_vm->alloc(new CFuncObject(ic)));
		return addMember(m_vm->alloc(new MethodObject(calb, InstanceObjRef())), name);
	}

	ObjRef attr(const std::string &name) override;
	void   setattr(const std::string &name, const ObjRef &o) override {
        m_dict->v[name] = o;
	}

	std::string baseName() {
		if (m_base.isNull())
			return std::string();
		return m_base->m_name;
	}

	// build an instance object for the given C++ object. wrap an existing object, don't copy or own it
	template <typename T>
	InstanceObjRef instancePtr(T *v);
	template <typename T>
	InstanceObjRef instanceSharedPtr(std::shared_ptr<T> v);
	// same above, copy v by value. the returned instance holds its own copy.
	template <typename TBase, typename T> // use this if you want the instance to be treated as TBase
	InstanceObjRef instanceValue(const T &v);
	template <typename T>
	InstanceObjRef instanceValue(const T &v) {
		return instanceValue<T, T>(v);
	}

	InstanceObjRef createInstance();

public:
	StrDictObjRef        m_dict;
	PoolPtr<ClassObject> m_base;
	std::string          m_name;
	PyVM *               m_vm;
	PoolPtr<ICInstWrap>  m_cwrap; // if it's a wrapper for a C++ object this will not be nullptr
};

using ClassObjRef = PoolPtr<ClassObject>;

class ModuleObject : public Object //, public IAttrable
{
public:
	ModuleObject(const std::string &name, PyVM *vm)
		: Object(MODULE, IATTRABLE), m_name(name), m_vm(vm) {}
	void clear() override {
		m_globals.clear(); // map clear
	}

	ObjRef addGlobal(const ObjRef &o, const std::string &name) {
		m_globals[name] = o;
		return o;
	}
	ObjRef getGlobal(const std::string &name) {
		return lookup(m_globals, name);
	}
	void delGlobal(const std::string &name) {
		m_globals.erase(name);
	}

	ObjRef attr(const std::string &name) override {
		return tryLookup(m_globals, name);
	}

	void setattr(const std::string &name, const ObjRef &o) override {
		m_globals[name] = o;
	}

	ObjRef defIc(const std::string &name, const ICWrapPtr &ic);

	template <typename F>
	ObjRef def(const std::string &name, F func) {
		return defIc(name, makeCWrap(func, m_vm));
	}

	ClassObjRef emptyClass(const std::string &name);

	template <typename C>
	ClassObjRef class_(const std::string &name) {
		ClassObjRef ret = emptyClass(name);
		ret->m_cwrap    = m_vm->alloct<ICInstWrap>(new CInstWrapPtr<C>());
		return ret;
	}

	template <typename C, typename CT>
	ClassObjRef class_(const std::string &name, CT ctorDef) {
		ClassObjRef ret      = class_<C>(name);
		ret->m_cwrap->m_ctor = m_vm->alloct<ICtorWrap>(new CtorWrap<C, typename CT::t1>());
		return ret;
	}

	template <typename F>
	void forClasses(F func) {
		forNameDict<ClassObject>(m_globals, func);
	}

	std::string m_name;
	NameDict    m_globals;
	PyVM *      m_vm; // needed for implementation of shortcuts of object creations.
};

class InstanceObject : public Object //, public IAttrable
{
public:
	InstanceObject(const ClassObjRef &cls)
		: Object(INSTANCE, IATTRABLE), m_class(cls) {}
	void clear() override {
		m_class.reset();
		m_dict.clear(); // map clear
		m_cwrap.reset();
	}

	~InstanceObject() override = default;

	ObjRef attr(const std::string &name) override;
	void   setattr(const std::string &name, const ObjRef &o) override {
        m_dict[name] = o;
	}
	// attr without __getattr__ support
	ObjRef simple_attr(const std::string &name);

	ClassObjRef m_class;
	NameDict    m_dict;

	/// if it wraps a C++ instance this will not be nullptr
	/// assigned in ClassObject::instancePtr or in the class call (with the ctor wrapper)
	PoolPtr<ICInstWrap> m_cwrap;
};

template <typename C>
C *extractCInst(const ObjRef o) {
	auto ct = checked_cast<InstanceObject>(o)->m_cwrap.get();
	CHECK(ct != nullptr, "nullptr C instance");
	return dynamic_cast<CInstWrap<C> *>(ct)->ptr();
}

template <typename C>
std::shared_ptr<C> extractCSharedPtr(const ObjRef o) {
	auto ct = checked_cast<InstanceObject>(o)->m_cwrap.get();
	CHECK(ct != nullptr, "nullptr C instance");
	return dynamic_cast<CInstWrapSharedPtr<C> *>(ct)->getSharedPtr();
}

template <typename T>
struct Extract<T *> {
	T *operator()(const ObjRef &o) {
		CHECK(!o.isNull(), "Extract from nullptr ref");
		return extractCInst<T>(o);
	}
};

template <typename T>
InstanceObjRef ClassObject::instancePtr(T *v) {
	if (v == nullptr)
		return InstanceObjRef(); // nullptr pointer is None is empty reference
	InstanceObjRef i(createInstance());
	i->m_cwrap = static_pcast<ICInstWrap>(m_vm->alloc(new CInstWrapPtr<T>(v)));
	//CHECK(typeid(*m_cwrap) == typeid(*i->m_cwrap), "Can't wrap something different from my class");
	// this test is not sufficient since it doesn't check with the cwrap of possible base class. need to do a recursive test.
	// this will be caught in the duynamic-cast in the call to the cwrap method so we can go without it for now.
	return i;
}

template <typename T>
InstanceObjRef ClassObject::instanceSharedPtr(std::shared_ptr<T> v) {
	if (v == nullptr)
		return InstanceObjRef(); // nullptr pointer is None is empty reference
	InstanceObjRef i(createInstance());
	i->m_cwrap = static_pcast<ICInstWrap>(m_vm->alloc(new CInstWrapSharedPtr<T>(v)));
	//CHECK(typeid(*m_cwrap) == typeid(*i->m_cwrap), "Can't wrap something different from my class");
	// this test is not sufficient since it doesn't check with the cwrap of possible base class. need to do a recursive test.
	// this will be caught in the duynamic-cast in the call to the cwrap method so we can go without it for now.
	return i;
}

template <typename TBase, typename T>
InstanceObjRef ClassObject::instanceValue(const T &v) {
	InstanceObjRef i(createInstance());
	i->m_cwrap = static_pcast<ICInstWrap>(m_vm->alloc(new CInstWrapValue<T, TBase>(v)));
	return i;
}

// wrapper that contains methods of primitive objects like string
class PrimitiveAttrAdapter : public CallableObject {
public:
	PrimitiveAttrAdapter(ObjRef obj, const std::string &name, PyVM *vm)
		: CallableObject(PRIMITIVE_ADAPTER, ModuleObjRef()), m_obj(obj), m_name(name), m_vm(vm) {}

	void clear() override {
		CallableObject::clear();
		m_obj.reset();
	}

	ObjRef      call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override;
	std::string funcname() const override {
		return std::string(m_obj->typeName()) + "." + m_name;
	}

	static bool adaptedType(Type t) {
		switch (t) {
		case Object::STR:
		case Object::USTR:
		case Object::LIST:
		case Object::STRDICT:
		case Object::DICT:
			return true;
		default:
			return false;
		}
	}

	ObjRef      m_obj;
	std::string m_name;
	PyVM *      m_vm; // for object creation

private:
	template <typename TC>
	ObjRef stringMethod(const ObjRef &obj, const CallArgs::TPosVector &args);

	ObjRef stringMethodConv(const ObjRef &obj, CallArgs::TPosVector &args);
	void   checkArgCount(const ObjRef obj, const CallArgs::TPosVector &args, int c);
	ObjRef listMethod(const ObjRef &obj, CallArgs::TPosVector &args);
	template <typename TDict>
	ObjRef dictMethod(const ObjRef &obj, CallArgs::TPosVector &args);
};

class Builtins : public ModuleObject {
public:
	Builtins(PyVM *vm);
	ObjRef get(const std::string &name);
	void   add(const std::string &name, const ObjRef &v);

private:
	ObjRef create(const std::string &name);
};

class GeneratorObject : public CallableObject, public IIterator, public IIterable {
public:
	GeneratorObject(const CodeObjRef &cobj, ModuleObjRef module, PyVM *vm)
		: CallableObject(GENERATOR, module), m_f(vm, module, &m_locals), m_atStart(true)
	// Object is not copyable so this initialization of m_f with the address of a member is ok
	{
		m_f.setCode(cobj);
	}
	ObjRef call(Frame &from, Frame &frame, int posCount, int kwCount, const ObjRef &self) override {
		(void)frame;

		m_f.localsFromStack(from, self, posCount, kwCount);
		return ObjRef(this);
	}

	std::string funcname() const override {
		return "generator";
	}

	ObjRef iter(PyVM *_vm) override {
		(void)_vm;
		return ObjRef(this);
	}

	bool next(ObjRef &obj) override {
		if (!m_atStart)
			m_f.push(m_f.m_vm->makeNone()); // value back from yield, generator send() not supported yet
		m_atStart = false;
		obj       = m_f.run();
		// this is needed to tell the difference between a generator yielding None and the legitimate end of a function
		return (m_f.m_retslot == SLOT_YIELD);
	}

	void clear() override {
		m_locals.clear(); // map clear
		m_f.clear();      // Frame clear calls resets or vector clear
	}

private:
	NameDict m_locals;
	Frame    m_f;       // execution frame of the co-routine
	bool     m_atStart; // true if 'next' was not called yet
};

class XRange : public Object, public IIterator, public IIterable {
public:
	XRange(int begin, int end, int step, PyVM *vm)
		: Object(XRANGE), m_begin(begin), m_end(end), m_step(step), m_next(begin), m_vm(vm) {}

	ObjRef iter(PyVM *_vm) override {
		return ObjRef(this);
	}

	bool next(ObjRef &obj) override {
		if (m_step > 0 && m_next >= m_end)
			return false;
		if (m_step < 0 && m_next <= m_end)
			return false;
		obj    = m_vm->makeFromT(m_next);
		m_next = m_next + m_step;
		return true;
	}

private:
	int   m_begin;
	int m_end;
	int m_step;
	int   m_next;
	PyVM *m_vm;
};

// ----- stuff from PyVM that needs the objects to be defined ---

template <typename T>
ObjRef PyVM::makeFromT(T v) {
	return alloc(new POBJ_TYPE(T)(v));
}

template <>
inline ObjRef PyVM::makeFromT(bool v) {
	if (v)
		return m_trueObject;
	return m_falseObject;
}

template <typename ObjT, typename InitT>
ObjRef PyVM::makeFromT2(const InitT &v) {
	return alloc(new ObjT(v));
}

template <typename A1, typename A2>
ObjRef PyVM::makeTuple(const A1 &a1, const A2 &a2) {
	auto t = alloct(new TupleObject());
	t->append(makeFromT(a1));
	t->append(makeFromT(a2));
	return ObjRef(t);
}

template <typename T> // T should be some ObjRef
void PyVM::addBuiltin(const std::string &name, const PoolPtr<T> &v) {
	m_builtins->add(name, ObjRef(v));
}

template <typename T>
T extract(const ObjRef &r) {
	return Extract<T>()(r);
}
