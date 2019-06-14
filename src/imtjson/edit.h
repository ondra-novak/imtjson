#pragma once


namespace json {

	class Object;
	class Array;

	template<typename Provider, typename Parent, typename Index>
	class AutoCommitT: public Provider {
	public:
		AutoCommitT(const Value &oldVal, Parent &parent, const Index &index)
			:Provider(oldVal),parent(parent),index(index) {}
		~AutoCommitT() {
			if (this->dirty()) {
				parent.set(index, *this);
			}
		}
		void commit() {
			if (this->dirty()) {
				parent.checkInstance();
				parent.set(index, *this);
				this->revert();
			}
		}
	protected:
		Parent &parent;
		Index index;
	};

	typedef AutoCommitT<Object, Object, StringView<char> > Object2Object;
	typedef AutoCommitT<Object, Array, std::size_t > Object2Array;
	typedef AutoCommitT<Array, Object, StringView<char> >  Array2Object ;
	typedef AutoCommitT<Array, Array, std::size_t > Array2Array;

}
