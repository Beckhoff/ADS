#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <condition_variable>
#include <mutex>

struct Semaphore
{
	void Post()
	{
		std::unique_lock<std::mutex> lock(mutex);
		++count;
		cv.notify_one();
	}

	void Stop()
	{
		running = false;
		Post();
	}

	bool Wait()
	{
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [&](){return count > 0; });
		--count;
		return running;
	}

private:
	bool running = true;
	int count = 0;
	std::mutex mutex;
	std::condition_variable cv;
};
#endif /* #ifndef _SEMAPHORE_H_ */