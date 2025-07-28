#ifndef NIMBLE_RMLOGRE_RESOURCEPOOL_HPP
#define NIMBLE_RMLOGRE_RESOURCEPOOL_HPP

#include <cassert>
#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>


namespace nimble::RmlOgre {

template <class T>
class ResourcePool
{
	std::vector<T> resources;
	std::deque<std::size_t> unused_;
	std::unordered_map<T, std::size_t> used_;

public:
	using Iterator = typename decltype(resources)::iterator;
	using ConstIterator = typename decltype(resources)::const_iterator;

	std::size_t size() const { return this->resources.size(); }
	Iterator begin() { return this->resources.begin(); }
	Iterator end() { return this->resources.end(); }
	ConstIterator begin() const { return this->resources.begin(); }
	ConstIterator end() const { return this->resources.end(); }

	std::size_t used() const { return this->used_.size(); }
	std::size_t unused() const { return this->unused_.size(); }
	bool full() const
	{
		return this->unused() == 0;
	}

	void add(T&& resource)
	{
		this->unused_.push_back(this->resources.size());
		this->resources.push_back(std::move(resource));
	}
	void add(const T& resource)
	{
		// Need move because MSVC incapable of handling r-value references properly
		this->add(std::move(T{resource}));
	}

	std::pair<T, std::size_t> claim()
	{
		assert(!this->unused_.empty());
		auto i = this->unused_.front();
		T& claimed = this->resources[i];
		this->unused_.pop_front();
		this->used_.insert({claimed, i});

		return {claimed, i};
	}
	bool free(T resource)
	{
		auto iter = this->used_.find(resource);
		if(iter == this->used_.end())
			return false;

		this->unused_.push_front(iter->second);
		this->used_.erase(iter);

		return true;
	}
};

}

#endif // NIMBLE_RMLOGRE_RESOURCEPOOL_HPP
