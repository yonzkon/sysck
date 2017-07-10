#ifndef DISK_H
#define DISK_H

#include <string>
#include <unistd.h>

namespace sysck {

template <class T,
		  class CONT,
		  template <class> class DetectPolicy,
		  template <class> class RecoverPolicy>
class disk : public DetectPolicy<T>,
			 public RecoverPolicy<T> {
public:
	typedef T partition_type;
	typedef CONT partition_collection_type;

private:
	disk(disk &rhs);
	disk& operator=(disk &rhs);

public:
	disk(std::string name)
		: name(name)
	{
		this->detect(name, partitions);
	}

	virtual ~disk() {}

public:
	const CONT& current_partitions() const
	{
		return partitions;
	}

	int reread_partition_table()
	{
		if (this->reread_table(this->name) == -1) {
			return -1;
		} else {
			sleep(1); // wait os to make device node file
			this->detect(name, partitions);
			return 0;
		}
	}

	void fsck_partitions(int timeout)
	{
		for (auto &item : partitions)
			this->fsck(item, timeout);
	}

private:
	std::string name;
	CONT partitions;
};

}

#endif
