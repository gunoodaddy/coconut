#include "CoconutLib.h"
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <vector>

using namespace coconut;

typedef boost::function< void () > func_t;
typedef boost::function< void (int) > argfunc_t;

class Test : public boost::enable_shared_from_this<Test> {
	public:
		Test() {
			printf("TEST!!!!!!!!\n");
		}
		~Test() {
			printf("~TEST!!!!!!!!\n");
		}

		void testBoostBind() {
			printf("TEST CALL\n");
		}

		void testArg1(int a) {
			printf("TEST CALL %d\n", a);
		}

		void call() {
			for(size_t i = 0; i < funcs_.size(); i++) {
				funcs_[i]();
			}
			funcs_.clear();

			argfunc_(10);
		}
		void testMain() {
			{
				func_t func_ = boost::bind(&Test::testBoostBind, shared_from_this());
				funcs_.push_back(func_);

				argfunc_ = boost::bind(&Test::testArg1, shared_from_this(), boost::arg<1>());

			}
			printf("-------------------------Test::testMain\n");
		}
		std::vector<func_t> funcs_;

		argfunc_t argfunc_;
};

class UpperTest
{
	public:
		UpperTest() {
			printf("UpperTest\n");
		}
		~UpperTest() {
			printf("~UpperTest\n");
		}
		void setTest(boost::shared_ptr<Test> test) {
			test_ = test;
		}

	private:
		boost::shared_ptr<Test> test_;
};


namespace ThreadTest {

	boost::recursive_mutex gLock;

	boost::recursive_mutex &getLock() {
		return gLock;
	}

	void func0() {
		std::cout << "func0" << std::endl;
		while(1) {

			boost::recursive_mutex &lock = getLock();
			lock.lock();
			boost::posix_time::seconds workTime(3); 	
			boost::this_thread::sleep(workTime); 
//			boost::xtime xt;
//			boost::xtime_get(&xt, boost::TIME_UTC);
//			xt.nsec += 3000000000;
//			boost::thread::sleep(xt);
			lock.unlock();

			printf("thread 0 running..\n");
		}
	}

	void func1() {
		std::cout << "func1" << std::endl;
		while(1) {
			boost::recursive_mutex &lock = getLock();
			lock.lock();
			boost::posix_time::seconds workTime(1); 	
			boost::this_thread::sleep(workTime); 
			lock.unlock();

			printf("thread 1 running..\n");
		}
	}

	void doTest() {
//		boost::thread::id a = boost::this_thread::get_id();
		boost::thread thread0(func0);
		boost::thread thread1(func1);

		//boost::recursive_mutex::scoped_lock autolock(mutex);
		thread0.join();
		thread1.join();
		printf("ThreadTest OK..\n");
	}
}

namespace templatefunc  {
	int * doSomething() {
		printf("doSomething\n");
		return new int;
	}

	template <class T>
	class BaseObjectAllocator
	{
	public:
		BaseObjectAllocator() { }
		~BaseObjectAllocator() { }
		typedef T * (*allocatorFunc_t)();
		typedef boost::shared_ptr<T> (*allocatorSharedFunc_t)();
		typedef void (*destorySharedFunc_t)(T *);

		static void setAllocator(allocatorFunc_t func) {
			allocator_ = func;	
		}
		static void setSharedAllocator(allocatorSharedFunc_t func) {
			allocatorShared_ = func;	
		}

		static void setSharedDestoryer(destorySharedFunc_t func) {
			destoryShared_ = func;
		}

		static boost::shared_ptr<T> makeSharedObject() {
			if(allocatorShared_)
				return allocatorShared_();
			return boost::shared_ptr<T>(new T, destoryShared_);
		}

		static T makeObject() {
			if(allocator_)
				return allocator_();
			return new T();
		}

	private:
		static allocatorFunc_t allocator_;
		static allocatorSharedFunc_t allocatorShared_;
		static destorySharedFunc_t destoryShared_;
	};

	template <class T> typename BaseObjectAllocator<T>::allocatorFunc_t         BaseObjectAllocator<T>::allocator_;
	template <class T> typename BaseObjectAllocator<T>::allocatorSharedFunc_t   BaseObjectAllocator<T>::allocatorShared_; 
	template <class T> typename BaseObjectAllocator<T>::destorySharedFunc_t     BaseObjectAllocator<T>::destoryShared_;

	void doTest() {
		BaseObjectAllocator<int>::setAllocator(&doSomething);
//		BaseObjectAllocator<int>::makeObject();
	}
}


int main() {
	// BLOCK START
	{
		templatefunc::doTest();
		boost::shared_ptr<UpperTest> null;
		return 1;
		if(null) {
			printf("shared_ptr not null\n");
		} else {
			printf("shared_ptr null\n");
		}

		boost::shared_ptr<UpperTest> upperTest(new UpperTest);
		boost::shared_ptr<Test> test(new Test);
		upperTest->setTest(test);
		test->testMain();
		test->call();

		ThreadTest::doTest();
	}
	printf("Program Exit..\n");
}
