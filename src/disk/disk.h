#ifndef DISK_H
#define DISK_H

#include <string>

namespace sysck {

template <class T,
		  template <class> class DetectPolicy,
		  template <class> class RecoverPolicy>
class disk : public DetectPolicy<T>,
			 public RecoverPolicy<T> {
public:
	typedef typename DetectPolicy<T>::partition_container partition_container;

private:
	disk(disk &rhs);
	disk& operator=(disk &rhs);

public:
	disk(std::string name)
		: name(name)
	{
		DetectPolicy<T>::detect(name, partitions);
	}

	virtual ~disk() {}

public:
	const partition_container& current_partitions() const
	{
		return partitions;
	}

	int reread_partition_table()
	{
		if (RecoverPolicy<T>::reread_table(this->name) == -1) {
			return -1;
		} else {
			DetectPolicy<T>::detect(name, partitions);
			return 0;
		}
	}

private:
	std::string name;
	partition_container partitions;
};

}

#endif
