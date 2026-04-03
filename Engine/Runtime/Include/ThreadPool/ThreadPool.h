
#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <cassert>
#include <iostream>
#include <string>
#include <chrono>

enum class ETaskPriority : uint8_t
{
	Lowest  = 0,
	Low     = 1,
	Normal  = 2,
	High    = 3,
	Highest = 4,
};


class IQueueWork 
{
public:
	virtual ~IQueueWork() = default;
	virtual void DoThreadWork() = 0;
	virtual void Abandon() { }
	virtual ETaskPriority GetPriority() const 
	{
		return ETaskPriority::Normal;
	}
};

class ThreadPool
{
public:
	enum class EShutDownMode
	{
		WaitForAll,		// 等待所有排队任务执行完毕
		DiscardPending, // 丢弃未执行的任务 (调用 Abandon)
	};

	// ThreadPool() = default;
	explicit ThreadPool(
		size_t threads_num = std::thread::hardware_concurrency() * 2,
		const std::string& name = "DefaultThreadPool"
	)
		: m_thread_pool_name(name)
		, m_worker_threads(threads_num)
		, m_stop(false)
		, m_active_tasks_num(0)
		, m_completed_tasks_num(0)
	{
		if(threads_num == 0)
		{
			threads_num = 2;
		}
		Create(threads_num);
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	~ThreadPool()
	{
		Destroy(EShutDownMode::WaitForAll);
	}

	template <typename Func, typename... Args>
	auto Submit(ETaskPriority priority, Func&& f, Args&&... args)
		-> std::future<std::invoke_result_t<Func, Args...>>
	{
		using ReturnType = std::invoke_result_t<Func, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
		);

		std::future<ReturnType> result = task->get_future();
		{
			std::lock_guard<std::mutex> lock(m_queue_mutex);
			if (m_stop) {
				throw std::runtime_error("ThreadPool is stopped. Cannot submit new tasks.");
			}
			m_task_queue.emplace(TaskWrapper{
				[task]()
				{
					(*task)();
				},
				priority
			});
			m_condition.notify_one();
		}
		return result;
	}

	template <typename Func, typename... Args>
	auto Submit(Func&& f, Args&&... args)
		-> std::future<std::invoke_result_t<Func, Args...>>
	{
		return Submit(ETaskPriority::Normal, std::forward<Func>(f), std::forward<Args>(args)...);	
	}

	void AddQueuedWork(IQueueWork* work)
	{
		if (!work)
		{
			return;
		}

		std::lock_guard<std::mutex> lock(m_queue_mutex);
		if (m_stop)
		{
			work->Abandon();
			delete work;
			return;
		}

		m_task_queue.emplace(TaskWrapper{
			[work]()
			{
				try
				{
					work->DoThreadWork();
				} 
				catch (...) 
				{
					// 防止异常逃逸到工作线程 
				}
				delete work;
			},
			work->GetPriority()
		});
		m_condition.notify_one();
	}

	void WaitForIdle()
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		m_idle_condition.wait(lock, [this]()
		{
			return m_task_queue.empty() && m_active_tasks_num.load() == 0;
		});
	}

	void Destroy(EShutDownMode mode = EShutDownMode::WaitForAll)
	{
		{
			std::lock_guard<std::mutex> lock(m_queue_mutex);
			if (m_stop)
			{
				return;
			}
			
			if (mode == EShutDownMode::DiscardPending)
			{
				while (!m_task_queue.empty())
				{
					auto& task_wrapper = m_task_queue.top();
					task_wrapper.run(); // 调用 Abandon
					m_task_queue.pop();
				}
			}
			m_stop = true;
		}
		m_condition.notify_all();

		for (std::thread& worker : m_worker_threads)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}
	}

	size_t GetThreadCount() const
	{
		return m_worker_threads.size();
	}

	size_t GetPendingTaskCount() const 
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		return m_task_queue.size();
	}

	const std::string& GetName() const
	{
		return m_thread_pool_name;
	}

	size_t GetCompletedTasks() const 
	{ 
		return m_completed_tasks_num.load(); 
	}

	bool IsStopped() const
	{
		return m_stop.load();
	}
private:

	struct TaskWrapper
	{
		std::function<void()> run;
		ETaskPriority priority;

		bool operator<(const TaskWrapper& other) const
		{
			// 优先级高的任务优先执行
			return static_cast<uint8_t>(priority) > static_cast<uint8_t>(other.priority); 
		}
	};

	void Create(size_t threads_num)
	{
		m_worker_threads.reserve(threads_num);
		for (size_t i = 0; i < threads_num; ++i)
		{
			m_worker_threads.emplace_back([this]()
			{
				WorkerLoop();
			});
		}
	}

	void WorkerLoop() 
	{
		while(true)
		{
			TaskWrapper task;
			{
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				m_condition.wait(lock, [this](){
					return m_stop.load() || !m_task_queue.empty();
				});

				if (m_stop && m_task_queue.empty())
				{
					return; // 线程池已停止且没有任务，退出线程
				}

				task = std::move(m_task_queue.top());
				m_task_queue.pop();
				m_active_tasks_num++;
			}

			try
            {
                task.run();
            }
            catch (const std::exception& e)
            {
                
            }
            catch (...)
            {
                
            }

			++ m_completed_tasks_num;
			-- m_active_tasks_num;

			m_idle_condition.notify_all();
		}
	}
private:
	std::string                      m_thread_pool_name;
	std::vector<std::thread>         m_worker_threads;
	std::priority_queue<TaskWrapper> m_task_queue;
	std::condition_variable          m_condition;
	std::condition_variable          m_idle_condition;
	std::atomic<bool>                m_stop { false };
	std::atomic<size_t>              m_active_tasks_num { 0 };
	std::atomic<size_t>              m_completed_tasks_num { 0 };
	mutable std::mutex               m_queue_mutex;
};

inline ThreadPool& GThreadPool()
{
	static ThreadPool global_thread_pool(std::thread::hardware_concurrency(), "GlobalThreadPool");
	return global_thread_pool;
}