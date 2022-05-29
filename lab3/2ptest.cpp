#pragma once
 
#include <queue>
#include <memory>
 
 
template <typename T>
class threadsafe_queue 
{
private:
	struct node {
		std::shared_ptr<T> data;
 
		//为什么要用unique_ptr,如果想做细粒度的锁控制,那么unique_ptr是必须的,细粒度控制意味着局部加锁,且各个动作之前应当是互不干扰的
		//比如pop只操作队列头部,push只操作队列尾部,这样的话就可以保证生产者线程(调用push的线程)和消费者线程(调用pop的线程)可以完全并发
		//进行。但是多个生产者之间还是需要同步,多个生产者之间也需要互相同步,且同步使用的就是局部锁。
		//unique_ptr可以保证指针不会被导出,这样的话外部就无法对指针额外加锁和销毁指针。
		//局部锁是并发访问的理论基础。
		//并发访问的核心思想就是通过为固有/常见的数据结构添加一些辅助节点,以实现生产者和消费者的访问隔离,在此基础上,如果有多个生产者和多个
		//消费者,则只提供生产者同步锁和消费者同步锁即可。
		std::unique_ptr<node> next;			
	};
 
	std::mutex head_m;
	std::mutex tail_m;
	std::unique_ptr<node> head;
	node* tail;
 
	node* get_tail() 
	{
		std::lock_guard<std::mutex> lock_tail(tail_m);
		return tail;
	}
 
	std::unique_ptr<node> pop_head()
	{
		std::lock_guard<std::mutex> lock_head(head_m);
 
		if (head.get() == get_tail()) {
			return nullptr;
		}
 
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}
 
public:
	threadsafe_queue()
		:head(new node)
		, tail(head.get())
	{}
 
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
 
	std::shared_ptr<T> try_pop() 
	{
		std::unique_ptr<node> old_head = pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}
 
	void push(T new_value) 
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<node> p(new node);
		node* const new_tail = p.get();
		std::lock_guard<std::mutex> tail_lock(tail_m);
		tail->data = new_data;
		tail->next = std::move(p);
		tail = new_tail;
	}
};
 
 
 
 
 
 
 
 
 