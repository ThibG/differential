#ifndef ABSTRACT1_H
#define ABSTRACT1_H

#include <ostream>
#include <map>
#include <set>
#include <string>
#include <sstream>
using namespace std;

#include "apronxx/apronxx.hh"
using namespace apron;

#include <llvm/Support/raw_ostream.h>

namespace differential {

class Abstract1 {


	/**
	 * To avoid duplication of memory consuming abstracts, we keep them all in one global map
	 * which maps the string print-out of the state to the state itself (this is the best way to truly avoid duplication)
	 */
	static map<string,const abstract1*> abstract_dictionary;
	static map<const abstract1*,set<var> > abstract_to_common_vars; // to avoid recomputing common vars
	static map<const abstract1*,set<var> > abstract_to_nonequiv_vars; // to avoid recomputing equivalence
	static map<const abstract1*,string > abstract_to_string; // to avoid recomputing the print
	static Abstract1 AddAbstractToAll(const abstract1 &abstract);

	const abstract1 * abstract_ptr_;

	string ReplaceTagPrefix(string abstract_str) const;

public:

	Abstract1() : abstract_ptr_(0){ }
	Abstract1(const abstract1 *abstract_ptr) : abstract_ptr_(abstract_ptr) { }
	Abstract1(const abstract1 &abstract) { *this = AddAbstractToAll(abstract); }
	virtual ~Abstract1() { }

	operator abstract1() const { assert(abstract_ptr_); return *abstract_ptr_; }
	const abstract1 * abstract() const { return abstract_ptr_; }

	// for stl containers
	string key() const;
	bool operator<(const Abstract1& left) const { return key() < left.key(); }
	bool operator>(const Abstract1& left) const { return key() > left.key(); }

	operator string() const;

	const set<var>& CommonVars() const;
	const set<var>& NonEquivVars() const;

	friend ostream& operator<<(ostream& os, const Abstract1& abstract ) {
		os << (string)abstract;
		return os;
	}

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Abstract1& abstract ) {
		os << (string)abstract;
		return os;
	}

};

}

#endif // ABSTRACT1_H
