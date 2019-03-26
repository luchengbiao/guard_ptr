// guard_ptr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <thread>
#include <vector>
#include "src/guard_ptr.h"

using namespace guard;

class Test : public support_guard_ptr<Test>
{
public:
	void alive() const
	{
		std::cout << "i am still alive: " << this->refCount() << std::endl;
	}
};

class TestDerived : public Test
{};

class Test0
{
public:
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << std::boolalpha;

	TestDerived* test = new TestDerived;

	guard_ptr<Test> test_ptr(test);

	Test0* test0 = new Test0;

	//guard_ptr<Test0> test0_ptr0(test0);

	delete test0;

	test->alive();

	{
		guard_ptr<Test> test_ptr0(test);
		if (test_ptr0.isAlive())
		{
			test_ptr0->alive();
		}
		test_ptr0.clear();
		test->alive();
		test_ptr0 = test;

		guard_ptr<Test> test_ptr1(test);
		if (test_ptr1.isAlive())
		{
			test_ptr1->alive();
		}

		guard_ptr<Test> test_ptr2(test_ptr0);
		if (test_ptr2.isAlive())
		{
			test_ptr2->alive();
		}

		guard_ptr<Test> test_ptr3;
		if (test_ptr3.isAlive())
		{
			test_ptr3->alive();
		}

		test_ptr3 = test_ptr1;
		if (test_ptr3.isAlive())
		{
			test_ptr3->alive();
		}

		guard_ptr<Test> test_ptr4(std::move(test_ptr3));
		if (test_ptr3.isAlive())
		{
			test_ptr3->alive();
		}

		if (test_ptr4.isAlive())
		{
			test_ptr4->alive();
		}
	}

	std::vector<std::thread> vec_thread;
	for (int i = 0; i < 5; ++i)
	{
		std::thread thread([test]{
			guard_ptr<Test> test_ptr(test);
			while (test_ptr.isAlive())
			{
				//std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // test(verify) guard_ptr is not thread-safe
				//test_ptr->alive();
			}
		});

		vec_thread.emplace_back(std::move(thread));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "test_ptr.isAlive(): " << test_ptr.isAlive() << std::endl;
	std::cout << "test_ptr.refCount(): " << test_ptr.refCount() << std::endl;
	
	delete test;

	std::cout << "test_ptr.isAlive(): " << test_ptr.isAlive() << std::endl;
	std::cout << "test_ptr.refCount(): " << test_ptr.refCount() << std::endl;
	
	for (auto& thread : vec_thread)
	{
		thread.join();
	}

	system("pause");

	return 0;
}

