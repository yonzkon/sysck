#ifndef STORAGE_DISK_HPP
#define STORAGE_DISK_HPP

#include <string>

namespace sysck {

template <class T,
		  template <class> class DetectPolicy,
		  template <class> class RecoverPolicy>
class storage_disk : public DetectPolicy<T>,
					 public RecoverPolicy<T> {
public:
	typedef typename DetectPolicy<T>::Container container;

private:
	storage_disk(storage_disk &rhs);
	storage_disk& operator=(storage_disk &rhs);

public:
	storage_disk(std::string name)
	{
		DetectPolicy<T>::detect(name, partitions);
	}

	virtual ~storage_disk() {}

public:
	const container& current_partitions() const
	{
		return partitions;
	}

	int rebuild_partition_table()
	{
		return 0;
	}

	int reread_partition_table()
	{
		return 0;
	}

private:
	container partitions;
};

}

#endif
