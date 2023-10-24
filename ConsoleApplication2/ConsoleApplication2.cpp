#include <iostream>
#include "allocator.h"
#include <string>
#include <cstddef>
#include <new>
#include "logger.h"
#include "logger_concrete.h"
#include "logger_builder.h"
#include "logger_builder_concrete.h"
#include "allocator.h"
#include "allocator_sorted_list.h"
#include "allocator_fit_allocation.h"

class A
{
private:
    int _value;
    char* _ptr;
public:
    A(int value) : _value(value), _ptr(new char[10]) {}
    // 5 rules
    int get_value() const noexcept
    {
        return _value;
    }
};

int main() {

    logger_builder* builder = new logger_builder_concrete();
    auto* logger = builder
        ->add_console_stream(logger::severity::debug)
        ->add_file_stream("sorted list allocator trace logs.txt", logger::severity::trace)
        ->build();
    delete builder;

    allocator* alc = new allocator_sorted_list(10000, nullptr, logger, allocator_fit_allocation::allocation_mode::first_fit);

    try
    {
        void* ptr = alc->allocate(1000);
        void* ptr_2 = alc->reallocate(ptr, 2000);

        // TODO: work with memory...

        A* obj2 = new A(13);
        obj2->get_value();
        A* obj = reinterpret_cast<A*>(alc->allocate(sizeof(A)));
        new (obj) A(13);
        obj->get_value();

        // TODO: this is incorrect
        // auto obj_to_copy_from = A(13);
        // std::memcpy(obj, &obj_to_copy_from, sizeof(A));

        delete obj2;
        obj->~A();
        alc->deallocate(ptr_2);
    }
    catch (allocator::memory_exception const& ex)
    {
        std::cout << "memory of size 100 can't be allocated!!1!1" << std::endl;
    }


    delete alc;
    delete logger;

    int x;
    std::cin >> x;

    return 0;
}