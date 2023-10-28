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
#include "allocator_border_descriptors.h"

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

    std::cout << "Allocator - Sorted List:" << std::endl;
    logger_builder* builder_3 = new logger_builder_concrete();
    auto* logger_3 = builder_3
        ->add_console_stream(logger::severity::debug)
        ->add_file_stream("sorted list allocator trace logs.txt", logger::severity::trace)
        ->build();
    delete builder_3;

    allocator* alc_3 = new allocator_sorted_list(10000, nullptr, logger_3, allocator_fit_allocation::allocation_mode::first_fit);

    try
    {
        void* ptr_3 = alc_3->allocate(1000);
        void* ptr_3_2 = alc_3->reallocate(ptr_3, 2000);

        // TODO: work with memory...

        A* obj2 = new A(13);
        obj2->get_value();
        A* obj = reinterpret_cast<A*>(alc_3->allocate(sizeof(A)));
        new (obj) A(13);
        obj->get_value();

        // TODO: this is incorrect
        // auto obj_to_copy_from = A(13);
        // std::memcpy(obj, &obj_to_copy_from, sizeof(A));

        delete obj2;
        obj->~A();
        alc_3->deallocate(ptr_3_2);
    }
    catch (allocator::memory_exception const& ex)
    {
        std::cout << "memory of size 100 can't be allocated!" << std::endl;
    }

    std::cout << std::endl << std::endl;

    //ЗАДАНИЕ 4
    std::cout << "Allocator - Descriptor:" << std::endl;

    logger_builder* builder_4 = new logger_builder_concrete();
    auto* logger_4 = builder_4
        ->add_console_stream(logger::severity::debug)
        ->add_file_stream("descriptor allocator trace logs.txt", logger::severity::trace)
        ->build();
    delete builder_4;

    allocator* alc2 = new allocator_border_descriptors(1200, alc_3, logger_4, allocator_fit_allocation::allocation_mode::first_fit);

    try
    {
        void* ptr_4 = alc2->allocate(900);
        ptr_4 = alc2->reallocate(ptr_4, 100);

        // TODO: work with memory...

        alc2->deallocate(ptr_4);
    }
    catch (allocator::memory_exception const& ex)
    {
        std::cout << "memory of size 100 can't be allocated!" << std::endl;
    }

    std::cout << std::endl << std::endl;
    delete alc_3;
    delete alc2;
    delete logger_4;


    int x;
    std::cin >> x;

    return 0;
}